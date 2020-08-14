/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

void ZePeak::ze_peak_dp_compute(L0Context &context) {
  long double gflops, timed;
  ze_result_t result = ZE_RESULT_SUCCESS;
  TimingMeasurement type = is_bandwidth_with_event_timer();
  size_t flops_per_work_item = 4096;
  struct ZeWorkGroups workgroup_info;
  double input_value = 1.3f;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_dp_compute.spv");

  context.create_module(binary_file);

  uint64_t max_work_items =
      get_max_work_items(context) * 512; // same multiplier in clPeak
  uint64_t max_number_of_allocated_items =
      context.device_property.maxMemAllocSize / sizeof(double);
  uint64_t number_of_work_items =
      MIN(max_number_of_allocated_items, (max_work_items * sizeof(double)));

  number_of_work_items =
      set_workgroups(context, number_of_work_items, &workgroup_info);

  void *device_input_value;
  ze_device_mem_alloc_desc_t in_device_desc = {};
  in_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  in_device_desc.pNext = nullptr;
  in_device_desc.ordinal = 0;
  in_device_desc.flags = 0;
  result = zeMemAllocDevice(context.context, &in_device_desc, sizeof(double), 1,
                            context.device, &device_input_value);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device input value allocated\n";

  void *device_output_buffer;
  ze_device_mem_alloc_desc_t out_device_desc = {};
  out_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  out_device_desc.pNext = nullptr;
  out_device_desc.ordinal = 0;
  out_device_desc.flags = 0;
  result = zeMemAllocDevice(
      context.context, &out_device_desc,
      static_cast<size_t>((number_of_work_items * sizeof(double))), 1,
      context.device, &device_output_buffer);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device output buffer allocated\n";

  result = zeCommandListAppendMemoryCopy(context.command_list,
                                         device_input_value, &input_value,
                                         sizeof(double), nullptr, 0, nullptr);
  if (result) {
    throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Input value copy encoded\n";

  result =
      zeCommandListAppendBarrier(context.command_list, nullptr, 0, nullptr);
  if (result) {
    throw std::runtime_error("zeCommandListAppendExecutionBarrier failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Execution barrier appended\n";

  context.execute_commandlist_and_sync();

  /*Begin setup of Functions*/

  ze_kernel_handle_t compute_dp_v1;
  setup_function(context, compute_dp_v1, "compute_dp_v1", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_dp_v2;
  setup_function(context, compute_dp_v2, "compute_dp_v2", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_dp_v4;
  setup_function(context, compute_dp_v4, "compute_dp_v4", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_dp_v8;
  setup_function(context, compute_dp_v8, "compute_dp_v8", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_dp_v16;
  setup_function(context, compute_dp_v16, "compute_dp_v16", device_input_value,
                 device_output_buffer);

  std::cout << "Double Precision Compute (GFLOPS)\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 1
  std::cout << "double : ";
  timed = run_kernel(context, compute_dp_v1, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 2
  std::cout << "double2 : ";
  timed = run_kernel(context, compute_dp_v2, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 4
  std::cout << "double4 : ";
  timed = run_kernel(context, compute_dp_v4, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 8
  std::cout << "double8 : ";
  timed = run_kernel(context, compute_dp_v8, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 16
  std::cout << "double16 : ";
  timed = run_kernel(context, compute_dp_v16, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  result = zeKernelDestroy(compute_dp_v1);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_dp_v1 Function Destroyed\n";

  result = zeKernelDestroy(compute_dp_v2);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_dp_v2 Function Destroyed\n";

  result = zeKernelDestroy(compute_dp_v4);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_dp_v4 Function Destroyed\n";

  result = zeKernelDestroy(compute_dp_v8);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_dp_v8 Function Destroyed\n";

  result = zeKernelDestroy(compute_dp_v16);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_dp_v16 Function Destroyed\n";

  result = zeMemFree(context.context, device_input_value);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Input Buffer freed\n";

  result = zeMemFree(context.context, device_output_buffer);
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
