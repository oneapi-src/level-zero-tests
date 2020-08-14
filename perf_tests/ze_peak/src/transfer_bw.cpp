/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

void ZePeak::_transfer_bw_gpu_copy(L0Context &context, void *destination_buffer,
                                   void *source_buffer, size_t buffer_size) {
  Timer timer;
  long double gbps = 0, timed = 0;
  ze_result_t result = ZE_RESULT_SUCCESS;

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    result = zeCommandListAppendMemoryCopy(context.command_list,
                                           destination_buffer, source_buffer,
                                           buffer_size, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }
    if (context.copy_command_list) {
      result = zeCommandListAppendMemoryCopy(context.copy_command_list,
                                             destination_buffer, source_buffer,
                                             buffer_size, nullptr, 0, nullptr);
      if (result) {
        std::cout << "Error appending to copy command list";
      }
    }
  }
  context.execute_commandlist_and_sync();
  if (context.copy_command_queue)
    context.execute_commandlist_and_sync(true);

  timer.start();
  for (uint32_t i = 0; i < iters; i++) {
    result = zeCommandListAppendMemoryCopy(context.command_list,
                                           destination_buffer, source_buffer,
                                           buffer_size, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }
  }

  context.execute_commandlist_and_sync();
  timed = timer.stopAndTime();
  timed /= static_cast<long double>(iters);

  gbps = calculate_gbps(timed, static_cast<long double>(buffer_size));

  std::cout << gbps << " GBPS\n";

  if (context.copy_command_queue) {
    timer.start();
    for (uint32_t i = 0; i < iters; i++) {
      result = zeCommandListAppendMemoryCopy(context.copy_command_list,
                                             destination_buffer, source_buffer,
                                             buffer_size, nullptr, 0, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                                 std::to_string(result));
      }
    }

    context.execute_commandlist_and_sync(true);
    timed = timer.stopAndTime();
    timed /= static_cast<long double>(iters);

    gbps = calculate_gbps(timed, static_cast<long double>(buffer_size));

    std::cout << "\t With Blitter Engine: " << gbps << " GBPS\n";
  }
}

void ZePeak::_transfer_bw_host_copy(L0Context &context,
                                    void *destination_buffer,
                                    void *source_buffer, size_t buffer_size,
                                    bool shared_is_dest) {
  Timer timer;
  long double gbps = 0, timed = 0;

  ze_command_list_handle_t temp_cmd_list = nullptr;
  ze_command_queue_desc_t cmd_q_desc = {};
  cmd_q_desc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  cmd_q_desc.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
  zeCommandListCreateImmediate(context.context, context.device, &cmd_q_desc,
                               &temp_cmd_list);

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    /*

    This test uses a shared memory buffer to measure transfer bandwidth
    between a host and device. The following helps to insure that the buffer is
    located on the device.

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

  std::cout << gbps << " GBPS\n";

  zeCommandListDestroy(temp_cmd_list);
}

void ZePeak::_transfer_bw_shared_memory(L0Context &context,
                                        std::vector<float> local_memory) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  void *shared_memory_buffer = nullptr;
  uint64_t number_of_items = local_memory.size();
  size_t local_memory_size =
      static_cast<size_t>(number_of_items * sizeof(float));

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;
  result = zeMemAllocShared(context.context, &device_desc, &host_desc,
                            local_memory_size, 1, context.device,
                            &shared_memory_buffer);
  if (result) {
    throw std::runtime_error("zeDriverAllocSharedMem failed: " +
                             std::to_string(result));
  }

  std::cout << "GPU Copy Host to Shared Memory : ";
  _transfer_bw_gpu_copy(context, shared_memory_buffer, local_memory.data(),
                        local_memory_size);

  std::cout << "GPU Copy Shared Memory to Host : ";
  _transfer_bw_gpu_copy(context, local_memory.data(), shared_memory_buffer,
                        local_memory_size);
  std::cout << "System Memory Copy to Shared Memory : ";
  _transfer_bw_host_copy(context, shared_memory_buffer, local_memory.data(),
                         local_memory_size, true);
  std::cout << "System Memory Copy from Shared Memory : ";
  _transfer_bw_host_copy(context, local_memory.data(), shared_memory_buffer,
                         local_memory_size, false);

  result = zeMemFree(context.context, shared_memory_buffer);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
}

void ZePeak::ze_peak_transfer_bw(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  uint64_t max_number_of_allocated_items =
      context.device_property.maxMemAllocSize / sizeof(float) / 2;
  uint64_t number_of_items = roundToMultipleOf(
      max_number_of_allocated_items,
      context.device_compute_property.maxGroupSizeX, transfer_bw_max_size);

  std::vector<float> local_memory(static_cast<uint32_t>(number_of_items));
  for (uint32_t i = 0; i < static_cast<uint32_t>(number_of_items); i++) {
    local_memory[i] = static_cast<float>(i);
  }

  size_t local_memory_size = (local_memory.size() * sizeof(float));

  void *device_buffer;
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  result =
      zeMemAllocDevice(context.context, &device_desc,
                       static_cast<size_t>(sizeof(float) * number_of_items), 1,
                       context.device, &device_buffer);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device buffer allocated\n";

  std::cout << "Transfer Bandwidth (GBPS)\n";

  std::cout << "enqueueWriteBuffer : ";
  _transfer_bw_gpu_copy(context, device_buffer, local_memory.data(),
                        local_memory_size);

  std::cout << "enqueueReadBuffer : ";
  _transfer_bw_gpu_copy(context, local_memory.data(), device_buffer,
                        local_memory_size);

  _transfer_bw_shared_memory(context, local_memory);

  result = zeMemFree(context.context, device_buffer);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Device Buffer freed\n";

  print_test_complete();
}
