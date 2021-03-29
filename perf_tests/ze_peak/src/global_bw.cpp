/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

void ZePeak::ze_peak_global_bw(L0Context &context) {
  long double timed_lo, timed_go, timed, gbps;
  ze_result_t result = ZE_RESULT_SUCCESS;
  uint64_t temp_global_size, max_total_work_items;
  struct ZeWorkGroups workgroup_info;
  TimingMeasurement type = is_bandwidth_with_event_timer();

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_global_bw.spv");

  context.create_module(binary_file);

  uint64_t maxItems =
      context.device_property.maxMemAllocSize / sizeof(float) / 2;
  uint64_t numItems = roundToMultipleOf(
      maxItems,
      (context.device_compute_property.maxGroupSizeX * FETCH_PER_WI * 16),
      global_bw_max_size);

  std::vector<float> arr(static_cast<uint32_t>(numItems));
  for (uint32_t i = 0; i < numItems; i++) {
    arr[i] = static_cast<float>(i);
  }

  if (context.sub_device_count) {
    numItems = numItems - (numItems % context.sub_device_count);
    if (verbose)
      std::cout << "splitting the total work items ::" << numItems
                << "across subdevices ::" << context.sub_device_count
                << std::endl;
    numItems = set_workgroups(context, numItems / context.sub_device_count,
                              &workgroup_info);
  } else {
    numItems = set_workgroups(context, numItems, &workgroup_info);
  }

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
      result = zeMemAllocDevice(context.context, &in_device_desc,
                                static_cast<size_t>((numItems * sizeof(float))),
                                1, device, &dev_in_val[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &in_device_desc,
                              static_cast<size_t>((numItems * sizeof(float))),
                              1, context.device, &inputBuf);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "inputBuf device buffer allocated\n";

  void *outputBuf;
  std::vector<void *> dev_out_val;
  ze_device_mem_alloc_desc_t out_device_desc = {};
  out_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  out_device_desc.pNext = nullptr;
  out_device_desc.ordinal = 0;
  out_device_desc.flags = 0;
  if (context.sub_device_count) {
    dev_out_val.resize(context.sub_device_count);
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      result = zeMemAllocDevice(context.context, &out_device_desc,
                                static_cast<size_t>((numItems * sizeof(float))),
                                1, device, &dev_out_val[i]);
      if (result) {
        throw std::runtime_error("zeMemAllocDevice failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeMemAllocDevice(context.context, &out_device_desc,
                              static_cast<size_t>((numItems * sizeof(float))),
                              1, context.device, &outputBuf);
    if (result) {
      throw std::runtime_error("zeMemAllocDevice failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "outputBuf device buffer allocated\n";

  if (context.sub_device_count) {
    for (auto i = 0; i < context.sub_device_count; i++) {
      result = zeCommandListAppendMemoryCopy(
          context.cmd_list[i], dev_in_val[i], arr.data(),
          ((arr.size() / context.sub_device_count) * sizeof(float)), nullptr, 0,
          nullptr);
      if (result) {
        throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                                 std::to_string(result));
      }
      i++;
    }
  } else {
    result = zeCommandListAppendMemoryCopy(
        context.command_list, inputBuf, arr.data(),
        (arr.size() * sizeof(float)), nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "Input buffer copy encoded\n";

  if (context.sub_device_count) {
    for (auto i = 0; i < context.sub_device_count; i++) {
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

  /*Begin setup of Function*/

  std::vector<ze_kernel_handle_t> lo_offset_v1;
  std::vector<ze_kernel_handle_t> lo_offset_v2;
  std::vector<ze_kernel_handle_t> lo_offset_v4;
  std::vector<ze_kernel_handle_t> lo_offset_v8;
  std::vector<ze_kernel_handle_t> lo_offset_v16;
  std::vector<ze_kernel_handle_t> gl_offset_v1;
  std::vector<ze_kernel_handle_t> gl_offset_v2;
  std::vector<ze_kernel_handle_t> gl_offset_v4;
  std::vector<ze_kernel_handle_t> gl_offset_v8;
  std::vector<ze_kernel_handle_t> gl_offset_v16;
  ze_kernel_handle_t local_offset_v1;
  ze_kernel_handle_t local_offset_v2;
  ze_kernel_handle_t local_offset_v4;
  ze_kernel_handle_t local_offset_v8;
  ze_kernel_handle_t local_offset_v16;
  ze_kernel_handle_t global_offset_v1;
  ze_kernel_handle_t global_offset_v2;
  ze_kernel_handle_t global_offset_v4;
  ze_kernel_handle_t global_offset_v8;
  ze_kernel_handle_t global_offset_v16;
  if (context.sub_device_count) {
    lo_offset_v1.resize(context.sub_device_count);
    lo_offset_v2.resize(context.sub_device_count);
    lo_offset_v4.resize(context.sub_device_count);
    lo_offset_v8.resize(context.sub_device_count);
    lo_offset_v16.resize(context.sub_device_count);
    gl_offset_v1.resize(context.sub_device_count);
    gl_offset_v2.resize(context.sub_device_count);
    gl_offset_v4.resize(context.sub_device_count);
    gl_offset_v8.resize(context.sub_device_count);
    gl_offset_v16.resize(context.sub_device_count);
    for (uint32_t i = 0; i < context.sub_device_count; i++) {

      setup_function(context, lo_offset_v1[i],
                     "global_bandwidth_v1_local_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, gl_offset_v1[i],
                     "global_bandwidth_v1_global_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, lo_offset_v2[i],
                     "global_bandwidth_v2_local_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, gl_offset_v2[i],
                     "global_bandwidth_v2_global_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, lo_offset_v4[i],
                     "global_bandwidth_v4_local_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, gl_offset_v4[i],
                     "global_bandwidth_v4_global_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, lo_offset_v8[i],
                     "global_bandwidth_v8_local_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, gl_offset_v8[i],
                     "global_bandwidth_v8_global_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, lo_offset_v16[i],
                     "global_bandwidth_v16_local_offset", dev_in_val[i],
                     dev_out_val[i]);

      setup_function(context, gl_offset_v16[i],
                     "global_bandwidth_v16_global_offset", dev_in_val[i],
                     dev_out_val[i]);
    }
  } else {

    setup_function(context, local_offset_v1, "global_bandwidth_v1_local_offset",
                   inputBuf, outputBuf);

    setup_function(context, global_offset_v1,
                   "global_bandwidth_v1_global_offset", inputBuf, outputBuf);

    setup_function(context, local_offset_v2, "global_bandwidth_v2_local_offset",
                   inputBuf, outputBuf);

    setup_function(context, global_offset_v2,
                   "global_bandwidth_v2_global_offset", inputBuf, outputBuf);

    setup_function(context, local_offset_v4, "global_bandwidth_v4_local_offset",
                   inputBuf, outputBuf);

    setup_function(context, global_offset_v4,
                   "global_bandwidth_v4_global_offset", inputBuf, outputBuf);

    setup_function(context, local_offset_v8, "global_bandwidth_v8_local_offset",
                   inputBuf, outputBuf);

    setup_function(context, global_offset_v8,
                   "global_bandwidth_v8_global_offset", inputBuf, outputBuf);

    setup_function(context, local_offset_v16,
                   "global_bandwidth_v16_local_offset", inputBuf, outputBuf);

    setup_function(context, global_offset_v16,
                   "global_bandwidth_v16_global_offset", inputBuf, outputBuf);
  }
  std::cout << "Global memory bandwidth (GBPS)\n";

  timed = 0;
  timed_lo = 0;
  timed_go = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 1
  std::cout << "float : ";

  // Run 2 kind of bandwidth kernel
  // lo -- local_size offset - subsequent fetches at local_size offset
  // go -- global_size offset
  temp_global_size = (numItems / FETCH_PER_WI);

  max_total_work_items =
      set_workgroups(context, temp_global_size, &workgroup_info);

  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed_lo += run_kernel(context, lo_offset_v1[i], workgroup_info, type);
      i++;
    }
    i = 0;
    for (auto device : context.sub_devices) {
      timed_go += run_kernel(context, gl_offset_v1[i], workgroup_info, type);
      i++;
    }
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;
    gbps = calculate_gbps(timed,
                          numItems * context.sub_device_count * sizeof(float));
    std::cout << gbps << " GFLOPS\n";
  } else {
    timed_lo = run_kernel(context, local_offset_v1, workgroup_info, type);
    timed_go = run_kernel(context, global_offset_v1, workgroup_info, type);
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;

    gbps = calculate_gbps(timed, numItems * sizeof(float));

    std::cout << gbps << " GBPS\n";
  }

  timed = 0;
  timed_lo = 0;
  timed_go = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 2
  std::cout << "float2 : ";

  temp_global_size = (numItems / 2 / FETCH_PER_WI);

  max_total_work_items =
      set_workgroups(context, temp_global_size, &workgroup_info);

  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed_lo += run_kernel(context, lo_offset_v2[i], workgroup_info, type);
      i++;
    }
    i = 0;
    for (auto device : context.sub_devices) {
      timed_go += run_kernel(context, gl_offset_v2[i], workgroup_info, type);
      i++;
    }
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;
    gbps = calculate_gbps(timed,
                          numItems * context.sub_device_count * sizeof(float));
    std::cout << gbps << " GFLOPS\n";
  } else {
    timed_lo = run_kernel(context, local_offset_v2, workgroup_info, type);
    timed_go = run_kernel(context, global_offset_v2, workgroup_info, type);
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;

    gbps = calculate_gbps(timed, numItems * sizeof(float));

    std::cout << gbps << " GBPS\n";
  }

  timed = 0;
  timed_lo = 0;
  timed_go = 0;

  ///////////////////////////////////////////////////////////////////////////
  // Vector width 4
  std::cout << "float4 : ";

  temp_global_size = (numItems / 4 / FETCH_PER_WI);

  max_total_work_items =
      set_workgroups(context, temp_global_size, &workgroup_info);

  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed_lo += run_kernel(context, lo_offset_v4[i], workgroup_info, type);
      i++;
    }
    i = 0;
    for (auto device : context.sub_devices) {
      timed_go += run_kernel(context, gl_offset_v4[i], workgroup_info, type);
      i++;
    }
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;
    gbps = calculate_gbps(timed,
                          numItems * context.sub_device_count * sizeof(float));
    std::cout << gbps << " GFLOPS\n";
  } else {
    timed_lo = run_kernel(context, local_offset_v4, workgroup_info, type);
    timed_go = run_kernel(context, global_offset_v4, workgroup_info, type);
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;

    gbps = calculate_gbps(timed, numItems * sizeof(float));

    std::cout << gbps << " GBPS\n";
  }

  timed = 0;
  timed_lo = 0;
  timed_go = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 8
  std::cout << "float8 : ";

  temp_global_size = (numItems / 8 / FETCH_PER_WI);

  max_total_work_items =
      set_workgroups(context, temp_global_size, &workgroup_info);

  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed_lo += run_kernel(context, lo_offset_v8[i], workgroup_info, type);
      i++;
    }
    i = 0;
    for (auto device : context.sub_devices) {
      timed_go += run_kernel(context, gl_offset_v8[i], workgroup_info, type);
      i++;
    }
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;
    gbps = calculate_gbps(timed,
                          numItems * context.sub_device_count * sizeof(float));
    std::cout << gbps << " GFLOPS\n";
  } else {
    timed_lo = run_kernel(context, local_offset_v8, workgroup_info, type);
    timed_go = run_kernel(context, global_offset_v8, workgroup_info, type);
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;

    gbps = calculate_gbps(timed, numItems * sizeof(float));

    std::cout << gbps << " GBPS\n";
  }

  timed = 0;
  timed_lo = 0;
  timed_go = 0;
  ///////////////////////////////////////////////////////////////////////////
  // Vector width 16
  std::cout << "float16 : ";
  temp_global_size = (numItems / 16 / FETCH_PER_WI);

  max_total_work_items =
      set_workgroups(context, temp_global_size, &workgroup_info);

  if (context.sub_device_count) {
    uint32_t i = 0;
    for (auto device : context.sub_devices) {
      timed_lo += run_kernel(context, lo_offset_v16[i], workgroup_info, type);
      i++;
    }
    i = 0;
    for (auto device : context.sub_devices) {
      timed_go += run_kernel(context, gl_offset_v16[i], workgroup_info, type);
      i++;
    }
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;
    gbps = calculate_gbps(timed,
                          numItems * context.sub_device_count * sizeof(float));
    std::cout << gbps << " GFLOPS\n";
  } else {
    timed_lo = run_kernel(context, local_offset_v16, workgroup_info, type);
    timed_go = run_kernel(context, global_offset_v16, workgroup_info, type);
    timed = (timed_lo < timed_go) ? timed_lo : timed_go;

    gbps = calculate_gbps(timed, numItems * sizeof(float));

    std::cout << gbps << " GBPS\n";
  }

  if (context.sub_device_count) {
    for (auto kernel : lo_offset_v1) {
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
    for (auto kernel : gl_offset_v1) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(global_offset_v1);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "global_offset_v1 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : lo_offset_v2) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(local_offset_v2);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "local_offset_v2 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : gl_offset_v2) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(global_offset_v2);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "global_offset_v2 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : lo_offset_v4) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(local_offset_v4);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "local_offset_v4 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : gl_offset_v4) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(global_offset_v4);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "global_offset_v4 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : lo_offset_v8) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(local_offset_v8);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "local_offset_v8 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : gl_offset_v8) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(global_offset_v8);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "global_offset_v8 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : lo_offset_v16) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(local_offset_v16);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "local_offset_v16 Function Destroyed\n";

  if (context.sub_device_count) {
    for (auto kernel : gl_offset_v16) {
      result = zeKernelDestroy(kernel);
      if (result) {
        throw std::runtime_error("zeKernelDestroy failed: " +
                                 std::to_string(result));
      }
    }
  } else {
    result = zeKernelDestroy(global_offset_v16);
    if (result) {
      throw std::runtime_error("zeKernelDestroy failed: " +
                               std::to_string(result));
    }
  }
  if (verbose)
    std::cout << "global_offset_v16 Function Destroyed\n";

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
    for (auto output_buf : dev_out_val) {
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
