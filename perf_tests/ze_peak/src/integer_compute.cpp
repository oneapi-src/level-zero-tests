/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

void ZePeak::ze_peak_int_compute(L0Context &context) {
  long double gflops, timed;
  ze_result_t result = ZE_RESULT_SUCCESS;
  TimingMeasurement type = is_bandwidth_with_event_timer();
  size_t flops_per_work_item = 2048;
  struct ZeWorkGroups workgroup_info;
  int input_value = 4;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_int_compute.spv");

  context.create_module(binary_file);

  uint64_t max_work_items =
      get_max_work_items(context) * 2048; // same multiplier in clPeak
  uint64_t max_number_of_allocated_items =
      max_device_object_size(context) / sizeof(int);
  uint64_t number_of_work_items =
      MIN(max_number_of_allocated_items, (max_work_items * sizeof(int)));

  number_of_work_items =
      set_workgroups(context, number_of_work_items, &workgroup_info);

  void *device_input_value;
  ze_device_mem_alloc_desc_t in_device_desc;
  in_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  in_device_desc.ordinal = 0;
  in_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  result = zeDriverAllocDeviceMem(context.driver, &in_device_desc, sizeof(int),
                                  1, context.device, &device_input_value);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device input value allocated\n";

  void *device_output_buffer;
  ze_device_mem_alloc_desc_t out_device_desc;
  out_device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  out_device_desc.ordinal = 0;
  out_device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  result = zeDriverAllocDeviceMem(
      context.driver, &out_device_desc,
      static_cast<size_t>((number_of_work_items * sizeof(int))), 1U,
      context.device, &device_output_buffer);
  if (result) {
    throw std::runtime_error("zeDriverAllocDeviceMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "device output buffer allocated\n";

  result =
      zeCommandListAppendMemoryCopy(context.command_list, device_input_value,
                                    &input_value, sizeof(int), nullptr);
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

  /*Begin setup of Function*/

  ze_kernel_handle_t compute_int_v1;
  setup_function(context, compute_int_v1, "compute_int_v1", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_int_v2;
  setup_function(context, compute_int_v2, "compute_int_v2", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_int_v4;
  setup_function(context, compute_int_v4, "compute_int_v4", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_int_v8;
  setup_function(context, compute_int_v8, "compute_int_v8", device_input_value,
                 device_output_buffer);
  ze_kernel_handle_t compute_int_v16;
  setup_function(context, compute_int_v16, "compute_int_v16",
                 device_input_value, device_output_buffer);

  std::cout << "Integer Compute (GFLOPS)\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 1
  std::cout << "int : ";
  timed = run_kernel(context, compute_int_v1, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 2
  std::cout << "int2 : ";
  timed = run_kernel(context, compute_int_v2, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 4
  std::cout << "int4 : ";
  timed = run_kernel(context, compute_int_v4, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 8
  std::cout << "int8 : ";
  timed = run_kernel(context, compute_int_v8, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 16
  std::cout << "int16 : ";
  timed = run_kernel(context, compute_int_v16, workgroup_info, type);
  gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
  std::cout << gflops << " GFLOPS\n";

  result = zeKernelDestroy(compute_int_v1);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_int_v1 Function Destroyed\n";

  result = zeKernelDestroy(compute_int_v2);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_int_v2 Function Destroyed\n";

  result = zeKernelDestroy(compute_int_v4);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_int_v4 Function Destroyed\n";

  result = zeKernelDestroy(compute_int_v8);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_int_v8 Function Destroyed\n";

  result = zeKernelDestroy(compute_int_v16);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "compute_int_v16 Function Destroyed\n";

  result = zeDriverFreeMem(context.driver, device_input_value);
  if (result) {
    throw std::runtime_error("zeDriverFreeMem failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Input Buffer freed\n";

  result = zeDriverFreeMem(context.driver, device_output_buffer);
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
