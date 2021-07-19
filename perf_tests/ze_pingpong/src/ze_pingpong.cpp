/*
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (C) 2019 Elias
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_pingpong.h"

//---------------------------------------------------------------------
// Utility function to initialize the ze driver, device, command list,
// command queue, & device property information.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void L0Context::init() {
  ze_command_list_desc_t command_list_description{};
  ze_command_queue_desc_t command_queue_description{};
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " + std::to_string(result));
  }

  uint32_t driver_count = 0;
  result = zeDriverGet(&driver_count, nullptr);
  if (result || driver_count == 0) {
    throw std::runtime_error("zeDriverGet failed: " + std::to_string(result));
  }

  /* Retrieve only one driver */
  driver_count = 1;
  result = zeDriverGet(&driver_count, &driver);
  if (result) {
    throw std::runtime_error("zeDriverGet failed: " + std::to_string(result));
  }

  /* Create a context to manage resources */
  ze_context_desc_t context_desc = {};
  context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
  result = zeContextCreate(driver, &context_desc, &context);
  if (ZE_RESULT_SUCCESS != result) {
    throw std::runtime_error("zeContextCreate failed: " +
                             std::to_string(result));
  }

  device_count = 0;
  result = zeDeviceGet(driver, &device_count, nullptr);
  if (result || device_count == 0) {
    throw std::runtime_error("zeDeviceGet failed: " + std::to_string(result));
  }

  device_count = 1;
  result = zeDeviceGet(driver, &device_count, &device);
  if (result) {
    throw std::runtime_error("zeDeviceGet failed: " + std::to_string(result));
  }

  device_property.pNext = nullptr;
  result = zeDeviceGetProperties(device, &device_property);
  if (result) {
    throw std::runtime_error("zeDeviceGetProperties failed: " +
                             std::to_string(result));
  }

  print_ze_device_properties(device_property);

  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.flags = static_cast<ze_command_list_flag_t>(0);
  command_list_description.pNext = nullptr;

  result = zeCommandListCreate(context, device, &command_list_description,
                               &command_list);
  if (result) {
    throw std::runtime_error("zeCommandListCreate failed: " +
                             std::to_string(result));
  }

  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.flags = 0;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = command_queue_id;
  command_queue_description.priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;

  result = zeCommandQueueCreate(context, device, &command_queue_description,
                                &command_queue);
  if (result) {
    throw std::runtime_error("zeCommandQueueCreate failed: " +
                             std::to_string(result));
  }

  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  device_desc.pNext = nullptr;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  result = zeMemAllocDevice(context, &device_desc, sizeof(int), 1, device,
                            &device_input);
  if (result) {
    throw std::runtime_error("zeMemAllocDevice failed: " +
                             std::to_string(result));
  }

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = 0;
  result = zeMemAllocHost(context, &host_desc, sizeof(int), 1, &host_output);
  if (result) {
    throw std::runtime_error("zeMemAllocHost failed: " +
                             std::to_string(result));
  }

  ze_device_mem_alloc_desc_t shared_device_desc = {};
  shared_device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;

  shared_device_desc.pNext = nullptr;
  shared_device_desc.ordinal = 0;
  shared_device_desc.flags = 0;
  ze_host_mem_alloc_desc_t shared_host_desc = {};
  shared_host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  shared_host_desc.pNext = nullptr;
  shared_host_desc.flags = 0;
  result = zeMemAllocShared(context, &shared_device_desc, &shared_host_desc,
                            sizeof(int), 1, device, &shared_output);
  if (result) {
    throw std::runtime_error("zeMemAllocShared failed: " +
                             std::to_string(result));
  }
}

//-----------------------------------------------------------------------------
// Utility function to close the command list & command queue and free buffers.
// On error, an exception will be thrown describing the failure.
//-----------------------------------------------------------------------------
void L0Context::destroy() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeCommandQueueDestroy(command_queue);
  if (result) {
    throw std::runtime_error("zeCommandQueueDestroy failed: " +
                             std::to_string(result));
  }

  result = zeCommandListDestroy(command_list);
  if (result) {
    throw std::runtime_error("zeCommandListDestroy failed: " +
                             std::to_string(result));
  }

  result = zeMemFree(context, device_input);
  if (result) {
    throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
  }

  result = zeMemFree(context, host_output);
  if (result) {
    throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
  }

  result = zeMemFree(context, shared_output);
  if (result) {
    throw std::runtime_error("zeMemFree failed: " + std::to_string(result));
  }

  result = zeContextDestroy(context);
  if (result) {
    throw std::runtime_error("zeContextDestroy failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Utility function to print the device properties from zeDeviceGetProperties.
//---------------------------------------------------------------------
void L0Context::print_ze_device_properties(
    const ze_device_properties_t &props) {

  std::cout << "Device : \n"
            << " * name                    : " << props.name << "\n"
            << " * vendorId                : " << std::hex << std::setfill('0')
            << std::setw(4) << props.vendorId << "\n"
            << " * deviceId                : " << std::hex << std::setfill('0')
            << std::setw(4) << props.deviceId << "\n"
            << " * subdeviceId             : " << props.subdeviceId << "\n"
            << " * isSubdevice             : "
            << ((props.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) ? "TRUE"
                                                                  : "FALSE")
            << "\n"
            << " * coreClockRate           : " << props.coreClockRate << "\n"
            << std::endl;
}

//---------------------------------------------------------------------
// Utility function to load the binary spv file from a path
// and return the file as a vector for use by L0.
//---------------------------------------------------------------------
std::vector<uint8_t> L0Context::load_binary_file(const std::string &file_path) {

  std::ifstream stream(file_path, std::ios::in | std::ios::binary);

  std::vector<uint8_t> binary_file;
  if (!stream.good()) {
    std::cerr << "Failed to load binary file: " << file_path << "\n";
    return binary_file;
  }

  size_t length = 0;
  stream.seekg(0, stream.end);
  length = static_cast<size_t>(stream.tellg());
  stream.seekg(0, stream.beg);

  binary_file.resize(length);
  stream.read(reinterpret_cast<char *>(binary_file.data()), length);

  return binary_file;
}

//---------------------------------------------------------------------
// Utility function to create the L0 module from a binary file.
// If successful, this function will set the context's module
// handle to a valid value for use in future calls.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePingPong::create_module(L0Context &l0_context,
                               std::vector<uint8_t> binary_file,
                               ze_module_format_t format,
                               const char *build_flag) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

  module_description.pNext = nullptr;
  module_description.format =
      format; // ZE_MODULE_FORMAT_IL_SPIRV or ZE_MODULE_FORMAT_NATIVE
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());

  module_description.pInputModule =
      reinterpret_cast<const uint8_t *>(binary_file.data());
  module_description.pBuildFlags = build_flag;

  result = zeModuleCreate(l0_context.context, l0_context.device,
                          &module_description, &l0_context.module, nullptr);
  if (result) {
    throw std::runtime_error("zeModuleCreate failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Utility function to set argument for kernel.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePingPong::set_argument_value(L0Context &context, uint32_t argIndex,
                                    size_t argSize, const void *pArgValue) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  result =
      zeKernelSetArgumentValue(context.function, argIndex, argSize, pArgValue);
  if (result) {
    throw std::runtime_error("zeKernelSetArgumentValue failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Utility function to execute the command lists on the command queue.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePingPong::run_command_queue(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  result = zeCommandQueueExecuteCommandLists(context.command_queue, 1,
                                             &context.command_list, nullptr);
  if (result) {
    throw std::runtime_error("zeCommandQueueExecuteCommandLists failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Utility function to synchronize the command queue.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePingPong::synchronize_command_queue(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  result = zeCommandQueueSynchronize(context.command_queue, UINT64_MAX);
  if (result) {
    throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Utility function to reset the Command List.
//---------------------------------------------------------------------
void ZePingPong::reset_commandlist(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeCommandListReset(context.command_list);
  if (result) {
    throw std::runtime_error("zeCommandListReset failed: " +
                             std::to_string(result));
  }
}

void ZePingPong::verify_result(int result) {
  const int validResult = 0;
  if (result == validResult)
    std::cout << "PASSED  ";
  else
    std::cout << "FAILED (" << result << "!=" << validResult << ")!\n";
}

void ZePingPong::setup_commandlist(L0Context &context, enum TestType test) {

  ze_result_t result = ZE_RESULT_SUCCESS;

  if (test == DEVICE_MEM_XFER) {
    result = zeCommandListAppendMemoryCopy(
        context.command_list, context.device_input, context.host_output,
        sizeof(int), nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }

    result =
        zeCommandListAppendBarrier(context.command_list, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendBarrier failed: " +
                               std::to_string(result));
    }

    result = zeCommandListAppendLaunchKernel(
        context.command_list, context.function,
        &context.thread_group_dimensions, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }

    result =
        zeCommandListAppendBarrier(context.command_list, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendBarrier failed: " +
                               std::to_string(result));
    }

    result = zeCommandListAppendMemoryCopy(
        context.command_list, context.host_output, context.device_input,
        sizeof(int), nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendMemoryCopy failed: " +
                               std::to_string(result));
    }

  } else {

    result = zeCommandListAppendLaunchKernel(
        context.command_list, context.function,
        &context.thread_group_dimensions, nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }
  }

  result = zeCommandListClose(context.command_list);
  if (result) {
    throw std::runtime_error("zeCommandListClose failed: " +
                             std::to_string(result));
  }
}

double ZePingPong::measure_benchmark(L0Context &context, enum TestType test) {

  int *ping = static_cast<int *>(context.device_input);
  int *pong = static_cast<int *>(context.host_output);
  int *ping_shared = static_cast<int *>(context.shared_output);

  pong[0] = 0;
  auto clk_begin = std::chrono::high_resolution_clock::now();
  auto clk_end = std::chrono::high_resolution_clock::now();
  if ((test == DEVICE_MEM_KERNEL_ONLY) || (test == HOST_MEM_KERNEL_ONLY) ||
      (test == SHARED_MEM_KERNEL_ONLY)) {
    clk_begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_execute; i++) {
      run_command_queue(context);
      // next line not needed for ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
      // synchronize_command_queue(context);
    }
    clk_end = std::chrono::high_resolution_clock::now();
  } else if ((test == DEVICE_MEM_XFER) || (test == HOST_MEM_NO_XFER)) {
    clk_begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_execute; i++) {
      run_command_queue(context);
      // next line not needed for ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
      // synchronize_command_queue(context);
      pong[0]--;
    }
    clk_end = std::chrono::high_resolution_clock::now();
    verify_result(pong[0]);
  } else if (test == SHARED_MEM_MAP) {
    clk_begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_execute; i++) {
      ping_shared[0] = pong[0];
      run_command_queue(context);
      // next line not needed for ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
      // synchronize_command_queue(context);
      pong[0] = ping_shared[0];
      pong[0]--;
    }

    clk_end = std::chrono::high_resolution_clock::now();
    verify_result(pong[0]);
  }
  auto elapsed_time =
      static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              clk_end - clk_begin)
                              .count()) /
      1000000.0;
  return elapsed_time;
}

void ZePingPong::run_test(L0Context &context) {

  ze_result_t result = ZE_RESULT_SUCCESS;

  std::vector<uint8_t> binary_file =
      context.load_binary_file("ze_pingpong.spv");

  create_module(context, binary_file, ZE_MODULE_FORMAT_IL_SPIRV, nullptr);

  ze_kernel_desc_t function_description = {};
  function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;

  int *ping = static_cast<int *>(context.device_input);
  int *pong = static_cast<int *>(context.host_output);
  int *ping_shared = static_cast<int *>(context.shared_output);

  function_description.pNext = nullptr;
  function_description.flags = 0;
  function_description.pKernelName = "kPingPong";

  result =
      zeKernelCreate(context.module, &function_description, &context.function);
  if (result) {
    throw std::runtime_error("zeKernelCreate failed: " +
                             std::to_string(result));
  }

  result = zeKernelSetGroupSize(context.function, 1, 1, 1);
  if (result) {
    throw std::runtime_error("zeKernelSetGroupSize failed: " +
                             std::to_string(result));
  }

  std::cout << "\n"
            << "KERNEL EXECUTION ONLY EXPERIMENTS\n\n";
  set_argument_value(context, 0, sizeof(ping), &ping);
  setup_commandlist(context, DEVICE_MEM_KERNEL_ONLY);

  // Warm-up
  for (int i = 0; i < num_execute / 2; i++) {
    run_command_queue(context);
    // next line not needed for ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS;
    // synchronize_command_queue(context);
  }
  reset_commandlist(context);
  set_argument_value(context, 0, sizeof(ping), &ping);
  setup_commandlist(context, DEVICE_MEM_KERNEL_ONLY);

  auto elapsed_time = measure_benchmark(context, DEVICE_MEM_KERNEL_ONLY);
  const auto loop_time_kernel_dev = elapsed_time / num_execute * 1000.;
  std::cout << "Device allocated memory w.sync:   " << std::fixed
            << std::setprecision(2) << loop_time_kernel_dev << " usec/loop ";
  std::cout << "(" << std::fixed << std::setprecision(2) << elapsed_time
            << " msec total)\n";
  reset_commandlist(context);

  set_argument_value(context, 0, sizeof(pong), &pong);
  setup_commandlist(context, HOST_MEM_KERNEL_ONLY);
  elapsed_time = measure_benchmark(context, HOST_MEM_KERNEL_ONLY);
  const auto loop_time_kernel_host = elapsed_time / num_execute * 1000.;
  std::cout << "Host allocated memory w.sync  :   " << std::fixed
            << std::setprecision(2) << loop_time_kernel_host << " usec/loop ";
  std::cout << "(" << std::fixed << std::setprecision(2) << elapsed_time
            << " msec total)\n";
  reset_commandlist(context);

  set_argument_value(context, 0, sizeof(ping_shared), &ping_shared);
  setup_commandlist(context, SHARED_MEM_KERNEL_ONLY);
  elapsed_time = measure_benchmark(context, SHARED_MEM_KERNEL_ONLY);
  const auto loop_time_kernel_shared = elapsed_time / num_execute * 1000.;
  std::cout << "Shared allocated memory w.sync:   " << std::fixed
            << std::setprecision(2) << loop_time_kernel_shared << " usec/loop ";
  std::cout << "(" << std::fixed << std::setprecision(2) << elapsed_time
            << " msec total)\n";
  reset_commandlist(context);

  std::cout << "\n"
            << "HOST->DEVICE EXPERIMENTS: CROSS-REFERENCE TO CLPINGPONG\n\n";

  std::cout << "CLPingPong Measurement                                    "
               "ZePingPong Measurement \n";
  std::cout << "---------------------------------------------------------------"
               "---------------------\n";
  std::cout << "Experiment #1:  Device allocated memory w. mapping        "
               "SHARED_MEM_MAP\n";
  std::cout << "Experiment #2:  Device allocated memory w. transfer       "
               "DEVICE_MEM_XFER\n";
  std::cout << "Experiment #3:  Host allocated memory w. mapping          "
               "SHARED_MEM_MAP\n";
  std::cout << "Experiment #4:  Host allocated memory w. transfer         "
               "HOST_MEM_NO_XFER\n\n";

  set_argument_value(context, 0, sizeof(ping_shared), &ping_shared);
  setup_commandlist(context, SHARED_MEM_MAP);
  std::cout << "SHARED_MEM_MAP   : ";
  elapsed_time = measure_benchmark(context, SHARED_MEM_MAP);
  const auto loop_time_shared_map = elapsed_time / num_execute * 1000.;
  std::cout << loop_time_shared_map << " usec/loop \n";
  reset_commandlist(context);

  set_argument_value(context, 0, sizeof(ping), &ping);
  setup_commandlist(context, DEVICE_MEM_XFER);
  std::cout << "DEVICE_MEM_XFER  : ";
  elapsed_time = measure_benchmark(context, DEVICE_MEM_XFER);
  const auto loop_time_dev_xfer = elapsed_time / num_execute * 1000.;
  std::cout << loop_time_dev_xfer << " usec/loop \n";
  reset_commandlist(context);

  set_argument_value(context, 0, sizeof(pong), &pong);
  setup_commandlist(context, HOST_MEM_NO_XFER);
  std::cout << "HOST_MEM_NO_XFER : ";
  elapsed_time = measure_benchmark(context, HOST_MEM_NO_XFER);
  const auto loop_time_host_noxfer = elapsed_time / num_execute * 1000.;
  std::cout << loop_time_host_noxfer << " usec/loop \n";

  auto min_ping_pong = std::min(loop_time_dev_xfer, loop_time_host_noxfer);
  min_ping_pong = std::min(min_ping_pong, loop_time_shared_map);
  auto loop_time_kernel = std::min(loop_time_kernel_dev, loop_time_kernel_host);
  loop_time_kernel = std::min(loop_time_kernel, loop_time_kernel_shared);
  std::cout << "\n"
            << "Host overhead: "
            << (100. * (min_ping_pong - loop_time_kernel)) / loop_time_kernel
            << "%"
            << "\n";

  result = zeKernelDestroy(context.function);
  if (result) {
    throw std::runtime_error("zeKernelDestroy failed: " +
                             std::to_string(result));
  }

  result = zeModuleDestroy(context.module);
  if (result) {
    throw std::runtime_error("zeModuleDestroy failed: " +
                             std::to_string(result));
  }
}

//---------------------------------------------------------------------
// Main function
//---------------------------------------------------------------------
int main(int argc, char **argv) {
  ZePingPong pingpong_benchmark;
  L0Context context;

  if (argc > 1) {
    throw std::runtime_error("no arguments are accepted");
  }

  context.init();

  pingpong_benchmark.run_test(context);

  context.destroy();

  std::cout << std::flush;

  return 0;
}
