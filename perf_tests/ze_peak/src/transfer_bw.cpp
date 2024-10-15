/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"
#include "../../common/include/common.hpp"

long double ZePeak::_transfer_bw_gpu_copy(L0Context &context,
                                          void *destination_buffer,
                                          void *source_buffer,
                                          size_t buffer_size) {
  Timer<std::chrono::nanoseconds::period> timer;
  long double gbps = 0;
  ze_result_t result = ZE_RESULT_SUCCESS;

  auto cmd_l = context.command_list;
  auto cmd_q = context.command_queue;

  if (context.copy_command_queue) {
    cmd_l = context.copy_command_list;
    cmd_q = context.copy_command_queue;
  } else if (context.sub_device_count) {
    cmd_l = context.cmd_list[current_sub_device_id];
    cmd_q = context.cmd_queue[current_sub_device_id];
  }

  SUCCESS_OR_TERMINATE(zeCommandListReset(cmd_l));
  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(cmd_l, destination_buffer,
                                                     source_buffer, buffer_size,
                                                     nullptr, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(cmd_l));

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    SUCCESS_OR_TERMINATE(
        zeCommandQueueExecuteCommandLists(cmd_q, 1, &cmd_l, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(cmd_q, UINT64_MAX));
  }

  timer.start();
  for (uint32_t i = 0; i < iters; i++) {
    SUCCESS_OR_TERMINATE(
        zeCommandQueueExecuteCommandLists(cmd_q, 1, &cmd_l, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(cmd_q, UINT64_MAX));
  }
  timer.end();
  double timed = timer.period_minus_overhead();
  timed /= static_cast<long double>(iters);

  return calculate_gbps(timed, static_cast<long double>(buffer_size));
}

long double ZePeak::_transfer_bw_host_copy(L0Context &context,
                                           void *destination_buffer,
                                           void *source_buffer,
                                           size_t buffer_size,
                                           bool shared_is_dest) {
  Timer<std::chrono::nanoseconds::period> timer;
  long double gbps = 0, timed = 0;

  ze_command_list_handle_t temp_cmd_list = nullptr;
  ze_command_queue_desc_t cmd_q_desc = {};
  cmd_q_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  cmd_q_desc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  if (context.sub_device_count) {
    zeCommandListCreateImmediate(context.context,
                                 context.sub_devices[current_sub_device_id],
                                 &cmd_q_desc, &temp_cmd_list);
  } else {
    if (enable_fixed_ordinal_index) {
      if (command_queue_group_ordinal >= context.queueProperties.size()) {
        std::cout << "Specified command queue group "
                  << command_queue_group_ordinal
                  << " is not valid, defaulting to first group" << std::endl;
      } else {
        cmd_q_desc.ordinal = command_queue_group_ordinal;
        if (command_queue_index <
            context.queueProperties[command_queue_group_ordinal].numQueues) {
          cmd_q_desc.index = command_queue_index;
        } else {
          cmd_q_desc.index = context.command_queue_id;
        }
      }
    }
    zeCommandListCreateImmediate(context.context, context.device, &cmd_q_desc,
                                 &temp_cmd_list);
  }

  /*
    Apply memory advise with preferred location set to system memory
    to ensure best placement for kmd-migrated shared allocations (only).
  */
  if (shared_is_dest) {
    zeCommandListAppendMemAdvise(
      temp_cmd_list, context.device, destination_buffer, buffer_size,
      ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
  } else {
    zeCommandListAppendMemAdvise(
      temp_cmd_list, context.device, source_buffer, buffer_size,
      ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
  }

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    /*

    This test uses a shared memory buffer to measure transfer bandwidth
    between a host and device. The following helps to insure that the
    buffer is located on the device.

    */

    uint8_t pattern = 0x0;
    size_t pattern_size = 1;
    zeCommandListAppendMemoryFill(
        temp_cmd_list, (shared_is_dest ? destination_buffer : source_buffer),
        &pattern, pattern_size, buffer_size, nullptr, 0, nullptr);

    memcpy(destination_buffer, source_buffer, buffer_size);
  }

  for (uint32_t i = 0; i < iters; i++) {
    uint8_t pattern = 0x0;
    size_t pattern_size = 1;
    zeCommandListAppendMemoryFill(
        temp_cmd_list, (shared_is_dest ? destination_buffer : source_buffer),
        &pattern, pattern_size, buffer_size, nullptr, 0, nullptr);

    timer.start();
    memcpy(destination_buffer, source_buffer, buffer_size);
    timed += timer.stopAndTime();
  }

  timed /= static_cast<long double>(iters);
  gbps = calculate_gbps(timed, static_cast<long double>(buffer_size));

  zeCommandListDestroy(temp_cmd_list);

  return gbps;
}

void ZePeak::_transfer_bw_shared_memory(L0Context &context,
                                        size_t local_memory_size,
                                        void *local_memory) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  long double gflops;
  void *shared_memory_buffer = nullptr;
  std::vector<void *> shared_buf;

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;
  if (context.sub_device_count) {
    shared_buf.resize(context.sub_device_count);
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      result = zeMemAllocShared(context.context, &device_desc, &host_desc,
                                local_memory_size / context.sub_device_count, 1,
                                device, &shared_buf[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocShared failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocShared(context.context, &device_desc, &host_desc,
                              local_memory_size, 1, context.device,
                              &shared_memory_buffer);
    if (result) {
      throw std::runtime_error("zeMemAllocShared failed: " +
                               std::to_string(result));
    }
  }

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops +=
          _transfer_bw_gpu_copy(context, shared_buf[i], local_memory,
                                local_memory_size / context.sub_device_count);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_gpu_copy(context, shared_memory_buffer, local_memory,
                                   local_memory_size);
  }
  std::cout << "GPU Copy Host to Shared Memory : ";
  std::cout << gflops << " GB/s\n";

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops +=
          _transfer_bw_gpu_copy(context, local_memory, shared_buf[i],
                                local_memory_size / context.sub_device_count);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_gpu_copy(context, local_memory, shared_memory_buffer,
                                   local_memory_size);
  }
  std::cout << "GPU Copy Shared Memory to Host : ";
  std::cout << gflops << " GB/s\n";

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops += _transfer_bw_host_copy(
          context, shared_buf[i], local_memory,
          local_memory_size / context.sub_device_count, true);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_host_copy(context, shared_memory_buffer, local_memory,
                                    local_memory_size, true);
  }
  std::cout << "System Memory Copy to Shared Memory : ";
  std::cout << gflops << " GB/s\n";

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops += _transfer_bw_host_copy(
          context, local_memory, shared_buf[i],
          local_memory_size / context.sub_device_count, false);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_host_copy(context, local_memory, shared_memory_buffer,
                                    local_memory_size, false);
  }
  std::cout << "System Memory Copy from Shared Memory : ";
  std::cout << gflops << " GB/s\n";

  current_sub_device_id = 0;

  if (context.sub_device_count) {
    for (auto output_buf : shared_buf) {
      result = zeMemFree(context.context, output_buf);
      if (result) {
        throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
      }
    }
  } else {

    result = zeMemFree(context.context, shared_memory_buffer);
    if (result) {
      throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
    }
  }
}

void ZePeak::ze_peak_transfer_bw(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  long double gflops;
  uint64_t max_number_of_allocated_items =
      context.device_property.maxMemAllocSize / sizeof(float) / 2;
  uint64_t number_of_items = roundToMultipleOf(
      max_number_of_allocated_items,
      context.device_compute_property.maxGroupSizeX, transfer_bw_max_size);
  size_t local_memory_size =
      static_cast<size_t>((number_of_items * sizeof(float)));
  void *host_memory = nullptr;
  ze_host_mem_alloc_desc_t host_desc = {};
  result = zeMemAllocHost(context.context, &host_desc, local_memory_size, 1,
                          &host_memory);
  if (result) {
    throw std::runtime_error("zeMemAllocHost failed: " +
                             std::to_string(result));
  }

  if (!host_memory) {
    throw std::runtime_error("Failed to allocate host memory");
  }
  float *local_memory = reinterpret_cast<float *>(host_memory);
  for (uint32_t i = 0; i < static_cast<uint32_t>(number_of_items); i++) {
    local_memory[i] = static_cast<float>(i);
  }

  void *device_buffer;
  std::vector<void *> dev_out_buf;
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  if (context.sub_device_count) {
    dev_out_buf.resize(context.sub_device_count);
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      result = zeMemAllocDevice(context.context, &device_desc,
                                local_memory_size / context.sub_device_count, 1,
                                device, &dev_out_buf[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &device_desc, local_memory_size,
                              1, context.device, &device_buffer);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "device buffer allocated\n";

  std::cout << "Transfer Bandwidth (GB/s)\n";

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops +=
          _transfer_bw_gpu_copy(context, dev_out_buf[i], host_memory,
                                local_memory_size / context.sub_device_count);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_gpu_copy(context, device_buffer, host_memory,
                                   local_memory_size);
  }
  std::cout << "enqueueWriteBuffer : ";
  std::cout << gflops << " GB/s\n";

  gflops = 0;
  if (context.sub_device_count) {
    current_sub_device_id = 0;
    for (auto i = 0; i < context.sub_device_count; i++) {
      gflops +=
          _transfer_bw_gpu_copy(context, host_memory, dev_out_buf[i],
                                local_memory_size / context.sub_device_count);
      current_sub_device_id++;
    }
    gflops = gflops / context.sub_device_count;
  } else {
    gflops = _transfer_bw_gpu_copy(context, host_memory, device_buffer,
                                   local_memory_size);
  }
  std::cout << "enqueueReadBuffer : ";
  std::cout << gflops << " GB/s\n";

  current_sub_device_id = 0;

  _transfer_bw_shared_memory(context, local_memory_size, local_memory);

  if (context.sub_device_count) {
    for (auto output_buf : dev_out_buf) {
      result = zeMemFree(context.context, output_buf);
      if (result) {
        throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
      }
    }
  } else {
    result = zeMemFree(context.context, device_buffer);
    if (result) {
      throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Device Buffer freed\n";

  result = zeMemFree(context.context, local_memory);
  if (result) {
    throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
  }
  if (verbose)
    std::cout << "Host Buffer freed\n";

  print_test_complete();
}
