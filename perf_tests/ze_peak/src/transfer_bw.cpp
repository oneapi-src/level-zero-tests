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
  long double gbps, timed;
  ze_result_t result = ZE_RESULT_SUCCESS;

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    result =
        zeCommandListAppendMemoryCopy(context.command_list, destination_buffer,
                                      source_buffer, buffer_size, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }
  }
  context.execute_commandlist_and_sync();

  timer.start();
  for (uint32_t i = 0; i < iters; i++) {
    result =
        zeCommandListAppendMemoryCopy(context.command_list, destination_buffer,
                                      source_buffer, buffer_size, nullptr);
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
}

void ZePeak::_transfer_bw_host_copy(void *destination_buffer,
                                    void *source_buffer, size_t buffer_size) {
  Timer timer;
  long double gbps, timed;

  for (uint32_t i = 0; i < warmup_iterations; i++) {
    memcpy(destination_buffer, source_buffer, buffer_size);
  }

  timer.start();
  for (uint32_t i = 0; i < iters; i++) {
    memcpy(destination_buffer, source_buffer, buffer_size);
  }
  timed = timer.stopAndTime();

  timed /= static_cast<long double>(iters);
  gbps = calculate_gbps(timed, static_cast<long double>(buffer_size));

  std::cout << gbps << " GBPS\n";
}

void ZePeak::_transfer_bw_shared_memory(L0Context &context,
                                        std::vector<float> local_memory) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  void *shared_memory_buffer = nullptr;
  uint64_t number_of_items = local_memory.size();
  size_t local_memory_size =
      static_cast<size_t>(number_of_items * sizeof(float));

  ze_device_mem_alloc_desc_t device_desc;
  device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  device_desc.ordinal = 0;
  device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  ze_host_mem_alloc_desc_t host_desc;
  host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
  host_desc.flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;
  result = zeDriverAllocSharedMem(context.driver, &device_desc, &host_desc,
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
  _transfer_bw_host_copy(shared_memory_buffer, local_memory.data(),
                         local_memory_size);
  std::cout << "System Memory Copy from Shared Memory : ";
  _transfer_bw_host_copy(local_memory.data(), shared_memory_buffer,
                         local_memory_size);

  result = zeDriverFreeMem(context.driver, shared_memory_buffer);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
}

void ZePeak::ze_peak_transfer_bw(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  uint64_t max_number_of_allocated_items =
      max_device_object_size(context) / sizeof(float) / 2;
  uint64_t number_of_items = roundToMultipleOf(
      max_number_of_allocated_items,
      context.device_compute_property.maxGroupSizeX, transfer_bw_max_size);

  std::vector<float> local_memory(static_cast<uint32_t>(number_of_items));
  for (uint32_t i = 0; i < static_cast<uint32_t>(number_of_items); i++) {
    local_memory[i] = static_cast<float>(i);
  }

  size_t local_memory_size = (local_memory.size() * sizeof(float));

  void *device_buffer;
  ze_device_mem_alloc_desc_t device_desc;
  device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  device_desc.ordinal = 0;
  device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  result = zeDriverAllocDeviceMem(
      context.driver, &device_desc,
      static_cast<size_t>(sizeof(float) * number_of_items), 1, context.device,
      &device_buffer);
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

  result = zeDriverFreeMem(context.driver, device_buffer);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Device Buffer freed\n";

  print_test_complete();
}
