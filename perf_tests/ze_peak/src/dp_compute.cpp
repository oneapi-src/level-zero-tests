/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

#ifndef EXCLUDE_MAIN
void ZePeak::ze_peak_dp_compute(L0Context &context) {
#else
std::vector<long double> ZePeak::ze_peak_dp_compute(L0Context &context) {
  std::vector<long double> gflops_list;
#endif
  long double gflops, timed;
  ze_result_t result = ZE_RESULT_SUCCESS;
  TimingMeasurement type = is_bandwidth_with_event_timer();
  size_t flops_per_work_item = 4096;
  struct ZeWorkGroups workgroup_info;
  double input_value = 1.3f;

  ze_device_module_properties_t module_properties{};
  module_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES;
  result = zeDeviceGetModuleProperties(context.device, &module_properties);
  if (!(module_properties.flags & ZE_DEVICE_MODULE_FLAG_FP64)) {
    std::cout << "dp_compute test skipping for missing support: "
              << "Device Module Properties not reporting "
                 "ZE_DEVICE_MODULE_FLAG_FP64\n";
#ifndef EXCLUDE_MAIN
    return;
#else
    return gflops_list;
#endif
  }

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_dp_compute.spv");

  context.create_module(binary_file);

  uint64_t max_work_items =
      get_max_work_items(context) * 512; // same multiplier in clPeak
  uint64_t max_number_of_allocated_items =
      context.device_property.maxMemAllocSize / sizeof(double);
  uint64_t number_of_work_items =
      MIN(max_number_of_allocated_items, (max_work_items * sizeof(double)));

  if (context.sub_device_count) {
    number_of_work_items = number_of_work_items -
                           (number_of_work_items % context.sub_device_count);
    if (verbose)
      std::cout << "splitting the total work items ::" << number_of_work_items
                << "across subdevices ::" << context.sub_device_count
                << std::endl;
    number_of_work_items =
        set_workgroups(context, number_of_work_items / context.sub_device_count,
                       &workgroup_info);
  } else {
    number_of_work_items =
        set_workgroups(context, number_of_work_items, &workgroup_info);
  }

  void *device_input_value;
  std::vector<void *> dev_in_val;
  ze_device_mem_alloc_desc_t in_device_desc = {};
  in_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  in_device_desc.pNext = nullptr;
  in_device_desc.ordinal = 0;
  in_device_desc.flags = 0;
  if (context.sub_device_count) {
    dev_in_val.resize(context.sub_device_count);
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      result = zeMemAllocDevice(context.context, &in_device_desc,
                                sizeof(double), 1, device, &dev_in_val[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &in_device_desc, sizeof(double),
                              1, context.device, &device_input_value);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "device input value allocated\n";

  void *device_output_buffer;
  std::vector<void *> dev_out_buf;
  ze_device_mem_alloc_desc_t out_device_desc = {};
  out_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  out_device_desc.pNext = nullptr;
  out_device_desc.ordinal = 0;
  out_device_desc.flags = 0;
  if (context.sub_device_count) {
    dev_out_buf.resize(context.sub_device_count);
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      result = zeMemAllocDevice(
          context.context, &out_device_desc,
          static_cast<size_t>((number_of_work_items * sizeof(double))), 1,
          device, &dev_out_buf[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(
        context.context, &out_device_desc,
        static_cast<size_t>((number_of_work_items * sizeof(double))), 1,
        context.device, &device_output_buffer);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "device output buffer allocated\n";

  if (context.sub_device_count) {
    for (uint32_t i = 0U; i < context.sub_device_count; i++) {
      result = zeCommandListAppendMemoryCopy(context.cmd_list[i], dev_in_val[i],
                                             &input_value, sizeof(double),
                                             nullptr, 0, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {

    result = zeCommandListAppendMemoryCopy(context.command_list,
                                           device_input_value, &input_value,
                                           sizeof(double), nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Input value copy encoded\n";

  if (context.sub_device_count) {
    for (uint32_t i = 0U; i < context.sub_device_count; i++) {
      result =
          zeCommandListAppendBarrier(context.cmd_list[i], nullptr, 0, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandListAppendBarrier failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result =
        zeCommandListAppendBarrier(context.command_list, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendBarrier failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Execution barrier appended\n";

  context.execute_commandlist_and_sync();

  /*Begin setup of Functions*/

  std::vector<ze_kernel_handle_t> dp_v1;
  std::vector<ze_kernel_handle_t> dp_v2;
  std::vector<ze_kernel_handle_t> dp_v4;
  std::vector<ze_kernel_handle_t> dp_v8;
  std::vector<ze_kernel_handle_t> dp_v16;
  ze_kernel_handle_t compute_dp_v1;
  ze_kernel_handle_t compute_dp_v2;
  ze_kernel_handle_t compute_dp_v4;
  ze_kernel_handle_t compute_dp_v8;
  ze_kernel_handle_t compute_dp_v16;
  if (context.sub_device_count) {
    dp_v1.resize(context.sub_device_count);
    dp_v2.resize(context.sub_device_count);
    dp_v4.resize(context.sub_device_count);
    dp_v8.resize(context.sub_device_count);
    dp_v16.resize(context.sub_device_count);
    for (uint32_t i = 0; i < context.sub_device_count; i++) {
      setup_function(context, dp_v1[i], "compute_dp_v1", dev_in_val[i],
                     dev_out_buf[i]);
      setup_function(context, dp_v2[i], "compute_dp_v2", dev_in_val[i],
                     dev_out_buf[i]);
      setup_function(context, dp_v4[i], "compute_dp_v4", dev_in_val[i],
                     dev_out_buf[i]);
      setup_function(context, dp_v8[i], "compute_dp_v8", dev_in_val[i],
                     dev_out_buf[i]);
      setup_function(context, dp_v16[i], "compute_dp_v16", dev_in_val[i],
                     dev_out_buf[i]);
    }
  } else {
    setup_function(context, compute_dp_v1, "compute_dp_v1", device_input_value,
                   device_output_buffer);

    setup_function(context, compute_dp_v2, "compute_dp_v2", device_input_value,
                   device_output_buffer);

    setup_function(context, compute_dp_v4, "compute_dp_v4", device_input_value,
                   device_output_buffer);

    setup_function(context, compute_dp_v8, "compute_dp_v8", device_input_value,
                   device_output_buffer);

    setup_function(context, compute_dp_v16, "compute_dp_v16",
                   device_input_value, device_output_buffer);
  }
  std::cout << "Double Precision Compute (GFLOPS)\n";

  timed = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 1
  std::cout << "double : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed += run_kernel(context, dp_v1[i], workgroup_info, type);
      i++;
    }
    gflops =
        calculate_gbps(timed, number_of_work_items * context.sub_device_count *
                                  flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  } else {
    timed = run_kernel(context, compute_dp_v1, workgroup_info, type);
    gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  }
#ifdef EXCLUDE_MAIN
  gflops_list.push_back(gflops);
#endif

  timed = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 2
  std::cout << "double2 : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed += run_kernel(context, dp_v2[i], workgroup_info, type);
      i++;
    }
    gflops =
        calculate_gbps(timed, number_of_work_items * context.sub_device_count *
                                  flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  } else {
    timed = run_kernel(context, compute_dp_v2, workgroup_info, type);
    gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  }
#ifdef EXCLUDE_MAIN
  gflops_list.push_back(gflops);
#endif

  timed = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 4
  std::cout << "double4 : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed += run_kernel(context, dp_v4[i], workgroup_info, type);
      i++;
    }
    gflops =
        calculate_gbps(timed, number_of_work_items * context.sub_device_count *
                                  flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  } else {
    timed = run_kernel(context, compute_dp_v4, workgroup_info, type);
    gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  }
#ifdef EXCLUDE_MAIN
  gflops_list.push_back(gflops);
#endif

  timed = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 8
  std::cout << "double8 : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed += run_kernel(context, dp_v8[i], workgroup_info, type);
      i++;
    }
    gflops =
        calculate_gbps(timed, number_of_work_items * context.sub_device_count *
                                  flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  } else {
    timed = run_kernel(context, compute_dp_v8, workgroup_info, type);
    gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  }
#ifdef EXCLUDE_MAIN
  gflops_list.push_back(gflops);
#endif

  timed = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 16
  std::cout << "double16 : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed += run_kernel(context, dp_v16[i], workgroup_info, type);
      i++;
    }
    gflops =
        calculate_gbps(timed, number_of_work_items * context.sub_device_count *
                                  flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  } else {
    timed = run_kernel(context, compute_dp_v16, workgroup_info, type);
    gflops = calculate_gbps(timed, number_of_work_items * flops_per_work_item);
    std::cout << gflops << " GFLOPS\n";
  }
#ifdef EXCLUDE_MAIN
  gflops_list.push_back(gflops);
#endif

  if (context.sub_device_count) {
    for (auto kernel : dp_v1) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(compute_dp_v1);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "compute_dp_v1 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : dp_v2) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(compute_dp_v2);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "compute_dp_v2 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : dp_v4) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(compute_dp_v4);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "compute_dp_v4 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : dp_v8) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(compute_dp_v8);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "compute_dp_v8 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : dp_v16) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(compute_dp_v16);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "compute_dp_v16 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto input_buf : dev_in_val) {
      result = zeMemFree(context.context, input_buf);
      if (result) {
        throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
      }
    }
  } else {
    result = zeMemFree(context.context, device_input_value);
    if (result) {
      throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Input Buffer freed\n";

  if (context.sub_device_count) {
    for (auto output_buf : dev_out_buf) {
      result = zeMemFree(context.context, output_buf);
      if (result) {
        throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
      }
    }
  } else {
    result = zeMemFree(context.context, device_output_buffer);
    if (result) {
      throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Output Buffer freed\n";

  if (context.sub_device_count) {
    for (auto module : context.subdevice_module) {
      result = zeModuleDestroy(module);
      if (result) {
        throw std::runtime_error("zeModuleDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeModuleDestroy(context.module);
    if (result) {
      throw std::runtime_error("zeModuleDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Module destroyed\n";

  print_test_complete();
#ifdef EXCLUDE_MAIN
  return gflops_list;
#endif
}
