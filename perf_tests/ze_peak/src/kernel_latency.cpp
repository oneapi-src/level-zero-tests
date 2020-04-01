/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

void ZePeak::ze_peak_kernel_latency(L0Context &context) {
  uint64_t num_items = get_max_work_items(context) * FETCH_PER_WI;
  uint64_t global_size = (num_items / FETCH_PER_WI);

  struct ZeWorkGroups workgroup_info;
  set_workgroups(context, global_size, &workgroup_info);
  long double latency = 0;
  ze_result_t result = ZE_RESULT_SUCCESS;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_global_bw.spv");

  context.create_module(binary_file);

  void *inputBuf;
  ze_device_mem_alloc_desc_t in_device_desc;
  in_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  in_device_desc.ordinal = 0;
  in_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  result =
      zeDriverAllocDeviceMem(context.driver, &in_device_desc,
                             static_cast<size_t>((num_items * sizeof(float))),
                             1, context.device, &inputBuf);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "inputBuf device buffer allocated\n";

  void *outputBuf;
  ze_device_mem_alloc_desc_t out_device_desc;
  out_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  out_device_desc.ordinal = 0;
  out_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  result =
      zeDriverAllocDeviceMem(context.driver, &out_device_desc,
                             static_cast<size_t>((num_items * sizeof(float))),
                             1, context.device, &outputBuf);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "outputBuf device buffer allocated\n";

  ze_kernel_handle_t local_offset_v1;
  setup_function(context, local_offset_v1, "global_bandwidth_v1_local_offset",
                 inputBuf, outputBuf);

  ///////////////////////////////////////////////////////////////////////////
  std::cout << "Kernel launch latency : ";
  latency = run_kernel(context, local_offset_v1, workgroup_info,
                       TimingMeasurement::KERNEL_LAUNCH_LATENCY, false);
  std::cout << latency << " (uS)\n";

  ///////////////////////////////////////////////////////////////////////////
  std::cout << "Kernel duration : ";
  latency = run_kernel(context, local_offset_v1, workgroup_info,
                       TimingMeasurement::KERNEL_COMPLETE_RUNTIME, false);
  std::cout << latency << " (uS)\n";

  result = zeKernelDestroy(local_offset_v1);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "local_offset_v1 Function Destroyed\n";

  result = zeDriverFreeMem(context.driver, inputBuf);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Input Buffer freed\n";

  result = zeDriverFreeMem(context.driver, outputBuf);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Output Buffer freed\n";

  result = zeModuleDestroy(context.module);
  if (result) {
    throw std::runtime_error("zeModuleDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Module destroyed\n";

  print_test_complete();
}
