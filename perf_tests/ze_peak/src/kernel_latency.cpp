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

  if (context.sub_device_count) {
    global_size = global_size - (global_size % context.sub_device_count);
    if (verbose)
      std::cout << "splitting the total work items ::" << num_items
                << "across subdevices ::" << context.sub_device_count
                << std::endl;
    set_workgroups(context, global_size / context.sub_device_count,
                   &workgroup_info);
  } else {
    set_workgroups(context, global_size, &workgroup_info);
  }

  long double latency = 0;
  ze_result_t result = ZE_RESULT_SUCCESS;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_global_bw.spv");

  context.create_module(binary_file);

  void *inputBuf;
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
      result = zeMemAllocDevice(
          context.context, &in_device_desc,
          static_cast<size_t>(
              ((num_items / context.sub_device_count) * sizeof(float))),
          1, device, &dev_in_val[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &in_device_desc,
                              static_cast<size_t>((num_items * sizeof(float))),
                              1, context.device, &inputBuf);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "inputBuf device buffer allocated\n";

  void *outputBuf;
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
          static_cast<size_t>(
              ((num_items / context.sub_device_count) * sizeof(float))),
          1, device, &dev_out_buf[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &out_device_desc,
                              static_cast<size_t>((num_items * sizeof(float))),
                              1, context.device, &outputBuf);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "outputBuf device buffer allocated\n";

  std::vector<ze_kernel_handle_t> compute_local_offset_v1;
  ze_kernel_handle_t local_offset_v1;
  if (context.sub_device_count) {
    compute_local_offset_v1.resize(context.sub_device_count);
    for (uint32_t i = 0; i < context.sub_device_count; i++) {
      setup_function(context, compute_local_offset_v1[i],
                     "global_bandwidth_v1_local_offset", dev_in_val[i],
                     dev_out_buf[i]);
    }
  } else {
    setup_function(context, local_offset_v1, "global_bandwidth_v1_local_offset",
                   inputBuf, outputBuf);
  }

  latency = 0;
  ///////////////////////////////////////////////////////////////////////////
  std::cout << "Kernel launch latency : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      latency += run_kernel(context, compute_local_offset_v1[i], workgroup_info,
                            TimingMeasurement::KERNEL_LAUNCH_LATENCY, true);
      i++;
    }
    std::cout << latency << " (us)\n";
  } else {
    latency = run_kernel(context, local_offset_v1, workgroup_info,
                         TimingMeasurement::KERNEL_LAUNCH_LATENCY, true);
    std::cout << latency << " (us)\n";
  }

  latency = 0;
  ///////////////////////////////////////////////////////////////////////////
  std::cout << "Kernel duration : ";
  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      latency += run_kernel(context, compute_local_offset_v1[i], workgroup_info,
                            TimingMeasurement::KERNEL_COMPLETE_RUNTIME, true);
      i++;
    }
    std::cout << latency << " (us)\n";
  } else {
    latency = run_kernel(context, local_offset_v1, workgroup_info,
                         TimingMeasurement::KERNEL_COMPLETE_RUNTIME, false);
    std::cout << latency << " (us)\n";
  }

  if (context.sub_device_count) {
    for (auto kernel : compute_local_offset_v1) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(local_offset_v1);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "local_offset_v1 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto input_buf : dev_in_val) {
      result = zeMemFree(context.context, input_buf);
      if (result) {
        throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
      }
    }
  } else {
    result = zeMemFree(context.context, inputBuf);
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
    result = zeMemFree(context.context, outputBuf);
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
}
