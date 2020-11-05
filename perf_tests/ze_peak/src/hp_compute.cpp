/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

// There is no equivalent of cl_half (i.e. 16 bit floating point)
// in the C/C++ standard. So we are just going to allocate 16 bits
// using a C type knowing it will be the same size.
#define cl_half uint16_t

void ZePeak::ze_peak_hp_compute(L0Context &context) {
  long double gflops, timed;
  ze_result_t result = ZE_RESULT_SUCCESS;
  TimingMeasurement type = is_bandwidth_with_event_timer();
  size_t flops_per_work_item = 4096;
  struct ZeWorkGroups workgroup_info;
  float input_value = 1.3f;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_hp_compute.spv");

  context.create_module(binary_file);

  // same multiplier in clPeak
  uint64_t max_work_items = get_max_work_items(context) * 2048;
  uint64_t max_number_of_allocated_items =
      context.device_property.maxMemAllocSize / sizeof(cl_half);
  uint64_t number_of_work_items =
      MIN(max_number_of_allocated_items, (max_work_items * sizeof(cl_half)));
  number_of_work_items =
      set_workgroups(context, number_of_work_items, &workgroup_info);

  void *device_output_buffer;
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  result = zeMemAllocDevice(
      context.context, &device_desc,
      static_cast<size_t>((number_of_work_items * sizeof(cl_half))), 1,
      context.device, &device_output_buffer);
  if (result) {
    throw std::runtime_error("zeMemAllocDevice failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device output buffer allocated\n";

  result =
      zeCommandListAppendBarrier(context.command_list, nullptr, 0, nullptr);
  if (result) {
    throw std::runtime_error("zeCommandListAppendBarrier failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Execution barrier appended\n";

  context.execute_commandlist_and_sync();

  /*Begin setup of Function*/

  ze_kernel_handle_t compute_hp_v1;
  setup_function(context, compute_hp_v1, "compute_hp_v1", device_output_buffer,
                 &input_value, sizeof(float));
  ze_kernel_handle_t compute_hp_v2;
  setup_function(context, compute_hp_v2, "compute_hp_v2", device_output_buffer,
                 &input_value, sizeof(float));
  ze_kernel_handle_t compute_hp_v4;
  setup_function(context, compute_hp_v4, "compute_hp_v4", device_output_buffer,
                 &input_value, sizeof(float));
  ze_kernel_handle_t compute_hp_v8;
  setup_function(context, compute_hp_v8, "compute_hp_v8", device_output_buffer,
                 &input_value, sizeof(float));
  ze_kernel_handle_t compute_hp_v16;
  setup_function(context, compute_hp_v16, "compute_hp_v16",
                 device_output_buffer, &input_value, sizeof(float));

  std::cout << "Half Precision Compute (GFLOPS)\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 1
  std::cout << "half : ";
  timed = run_kernel(context, compute_hp_v1, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 2
  std::cout << "half2 : ";
  timed = run_kernel(context, compute_hp_v2, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 4
  std::cout << "half4 : ";
  timed = run_kernel(context, compute_hp_v4, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 8
  std::cout << "half8 : ";
  timed = run_kernel(context, compute_hp_v8, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 16
  std::cout << "half16 : ";
  timed = run_kernel(context, compute_hp_v16, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  result = zeKernelDestroy(compute_hp_v1);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_hp_v1 Function Destroyed\n";

  result = zeKernelDestroy(compute_hp_v2);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_hp_v2 Function Destroyed\n";

  result = zeKernelDestroy(compute_hp_v4);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_hp_v4 Function Destroyed\n";

  result = zeKernelDestroy(compute_hp_v8);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_hp_v8 Function Destroyed\n";

  result = zeKernelDestroy(compute_hp_v16);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_hp_v16 Function Destroyed\n";

  result = zeMemFree(context.context, device_output_buffer);
  if (result) {
    throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
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
