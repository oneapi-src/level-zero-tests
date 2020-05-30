/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

#include <algorithm>

#define ONE_KB (1 * 1024ULL)
#define EIGHT_KB (8 * ONE_KB)
#define ONE_MB (1 * 1024ULL * ONE_KB)
#define FOUR_GB (4 * 1024ULL * ONE_MB)

//---------------------------------------------------------------------
// Utility function to load the binary spv file from a path
// and return the file as a vector for use by L0.
//---------------------------------------------------------------------
std::vector<uint8_t> L0Context::load_binary_file(const std::string &file_path) {
  if (verbose)
    std::cout << "File path: " << file_path << "\n";
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
  if (verbose)
    std::cout << "Binary file length: " << length << "\n";

  binary_file.resize(length);
  stream.read(reinterpret_cast<char *>(binary_file.data()), length);
  if (verbose)
    std::cout << "Binary file loaded"
              << "\n";

  return binary_file;
}

//---------------------------------------------------------------------
// Utility function to reset the Command List.
//---------------------------------------------------------------------
void L0Context::reset_commandlist() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeCommandListReset(command_list);
  if (result) {
    throw std::runtime_error("zeCommandListReset failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command list reset\n";
}

//---------------------------------------------------------------------
// Utility function to create the L0 module from a binary file.
// If successful, this function will set the context's module
// handle to a valid value for use in future calls.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void L0Context::create_module(std::vector<uint8_t> binary_file) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  ze_module_desc_t module_description = {};

  module_description.version = ZE_MODULE_DESC_VERSION_CURRENT;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule =
      reinterpret_cast<const uint8_t *>(binary_file.data());
  module_description.pBuildFlags = nullptr;

  result = zeModuleCreate(device, &module_description, &module, nullptr);
  if (result) {
    throw std::runtime_error("zeDeviceCreateModule failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Module created\n";
}

//---------------------------------------------------------------------
// Utility function to print the device properties from zeDeviceGetProperties.
//---------------------------------------------------------------------
void L0Context::print_ze_device_properties(
    const ze_device_properties_t &props) {
  std::cout << "Device : \n"
            << " * name : " << props.name << "\n"
            << " * vendorId : " << props.vendorId << "\n"
            << " * deviceId : " << props.deviceId << "\n"
            << " * subdeviceId : " << props.subdeviceId << "\n"
            << " * isSubdevice : " << (props.isSubdevice ? "TRUE" : "FALSE")
            << "\n"
            << " * coreClockRate : " << props.coreClockRate << "\n"
            << " * numAsyncComputeEngines : " << props.numAsyncComputeEngines
            << "\n"
            << " * numAsyncCopyEngines  : " << props.numAsyncCopyEngines << "\n"
            << " * maxCommandQueuePriority : " << props.maxCommandQueuePriority
            << std::endl;
}

//---------------------------------------------------------------------
// Utility function to initialize the ze driver, device, command list,
// command queue, & device property information.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void L0Context::init_xe() {
  ze_command_list_desc_t command_list_description{};
  ze_command_queue_desc_t command_queue_description{};
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeInit(ZE_INIT_FLAG_NONE);
  if (result) {
    throw std::runtime_error("zeDriverInit failed: " + std::to_string(result));
  }
  if (verbose)
    std::cout << "Driver initialized\n";

  std::cout << "zeDriverGet...\n";
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

  device_count = 0;
  result = zeDeviceGet(driver, &device_count, nullptr);
  if (result || device_count == 0) {
    throw std::runtime_error("zeDeviceGet failed: " + std::to_string(result));
  }
  if (verbose)
    std::cout << "Device count retrieved\n";

  device_count = 1;
  result = zeDeviceGet(driver, &device_count, &device);
  if (result) {
    throw std::runtime_error("zeDeviceGet failed: " + std::to_string(result));
  }
  if (verbose)
    std::cout << "Device retrieved\n";

  device_property.version = ZE_DEVICE_PROPERTIES_VERSION_CURRENT;
  result = zeDeviceGetProperties(device, &device_property);
  if (result) {
    throw std::runtime_error("zeDeviceGetProperties failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Device Properties retrieved\n";

  print_ze_device_properties(device_property);

  device_compute_property.version =
      ZE_DEVICE_COMPUTE_PROPERTIES_VERSION_CURRENT;
  result = zeDeviceGetComputeProperties(device, &device_compute_property);
  if (result) {
    throw std::runtime_error("zeDeviceGetComputeProperties failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Device Compute Properties retrieved\n";

  command_list_description.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;

  result =
      zeCommandListCreate(device, &command_list_description, &command_list);
  if (result) {
    throw std::runtime_error("zeDeviceCreateCommandList failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "command_list created\n";

  command_queue_description.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
  command_queue_description.ordinal = command_queue_id;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

  result =
      zeCommandQueueCreate(device, &command_queue_description, &command_queue);
  if (result) {
    throw std::runtime_error("zeDeviceCreateCommandQueue failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command queue created\n";
}

//---------------------------------------------------------------------
// Utility function to close the command list & command queue.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void L0Context::clean_xe() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeCommandQueueDestroy(command_queue);
  if (result) {
    throw std::runtime_error("zeCommandQueueDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command queue destroyed\n";

  result = zeCommandListDestroy(command_list);
  if (result) {
    throw std::runtime_error("zeCommandListDestroy failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "command_list destroyed\n";
}

//---------------------------------------------------------------------
// Utility function to execute the command list & synchronize
// the command queue. This function will reset the command list once the
// queue has been synchronized indicating that the commands in the command
// list have been completed.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void L0Context::execute_commandlist_and_sync() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  result = zeCommandListClose(command_list);
  if (result) {
    throw std::runtime_error("zeCommandListClose failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command list closed\n";

  result = zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list,
                                             nullptr);
  if (result) {
    throw std::runtime_error("zeCommandQueueExecuteCommandLists failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command list enqueued\n";

  result = zeCommandQueueSynchronize(command_queue, UINT32_MAX);
  if (result) {
    throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Command queue synchronized\n";

  reset_commandlist();
}

//---------------------------------------------------------------------
// Utility function to total the current work items that would be
// executed given x,y,z sizes and x,y,z counts for the workgroups.
//---------------------------------------------------------------------
uint64_t total_current_work_items(uint64_t group_size_x, uint64_t group_count_x,
                                  uint64_t group_size_y, uint64_t group_count_y,
                                  uint64_t group_size_z,
                                  uint64_t group_count_z) {
  return (group_size_x * group_count_x * group_size_y * group_count_y *
          group_size_z * group_count_z);
}

//---------------------------------------------------------------------
// Utility function to set the workgroup dimensions based on the desired
// number of work items a user wants to execute.
// This will attempt to distribute the work items across the workgroup
// dimensions and get to as close to the work items requested as possible.
// Once the number of work items that would be executed is equal to or >
// the number of work items requested, then the workgroup information
// is set accordingly and the total work items that will execute is returned.
//---------------------------------------------------------------------
uint64_t ZePeak::set_workgroups(L0Context &context,
                                const uint64_t total_work_items_requested,
                                struct ZeWorkGroups *workgroup_info) {

  auto group_size_x =
      std::min(total_work_items_requested,
               uint64_t(context.device_compute_property.maxGroupSizeX));
  auto group_size_y = 1;
  auto group_size_z = 1;
  auto group_size = group_size_x * group_size_y * group_size_z;

  auto group_count_x = total_work_items_requested / group_size;
  group_count_x = std::min(
      group_count_x, uint64_t(context.device_compute_property.maxGroupCountX));
  auto remaining_items =
      total_work_items_requested - group_count_x * group_size;

  uint64_t group_count_y = remaining_items / (group_count_x * group_size);
  group_count_y = std::min(
      group_count_y, uint64_t(context.device_compute_property.maxGroupCountY));
  group_count_y = std::max(group_count_y, uint64_t(1));
  remaining_items =
      total_work_items_requested - group_count_x * group_count_y * group_size;

  uint64_t group_count_z =
      remaining_items / (group_count_x * group_count_y * group_size);
  group_count_z = std::min(
      group_count_z, uint64_t(context.device_compute_property.maxGroupCountZ));
  group_count_z = std::max(group_count_z, uint64_t(1));

  auto final_work_items =
      group_count_x * group_count_y * group_count_z * group_size;
  remaining_items = total_work_items_requested - final_work_items;

  if (verbose) {
    std::cout << "Group size x: " << group_size_x << "\n";
    std::cout << "Group size y: " << group_size_y << "\n";
    std::cout << "Group size z: " << group_size_z << "\n";
    std::cout << "Group count x: " << group_count_x << "\n";
    std::cout << "Group count y: " << group_count_y << "\n";
    std::cout << "Group count z: " << group_count_z << "\n";
  }

  if (verbose)
    std::cout << "total work items that will be executed: " << final_work_items
              << " requested: " << total_work_items_requested << "\n";

  workgroup_info->group_size_x = static_cast<uint32_t>(group_size_x);
  workgroup_info->group_size_y = static_cast<uint32_t>(group_size_y);
  workgroup_info->group_size_z = static_cast<uint32_t>(group_size_z);
  workgroup_info->thread_group_dimensions.groupCountX =
      static_cast<uint32_t>(group_count_x);
  workgroup_info->thread_group_dimensions.groupCountY =
      static_cast<uint32_t>(group_count_y);
  workgroup_info->thread_group_dimensions.groupCountZ =
      static_cast<uint32_t>(group_count_z);

  return final_work_items;
}

//---------------------------------------------------------------------
// Utility function to execute the command lists on the command queue.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePeak::run_command_queue(L0Context &context) {
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
void ZePeak::synchronize_command_queue(L0Context &context) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  result = zeCommandQueueSynchronize(context.command_queue, UINT32_MAX);
  if (result) {
    throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                             std::to_string(result));
  }
}

void single_event_pool_create(
    L0Context &context, ze_event_pool_handle_t *kernel_launch_event_pool) {
  ze_result_t result;
  ze_event_pool_desc_t kernel_launch_event_pool_desc = {};

  kernel_launch_event_pool_desc.count = 1;
  kernel_launch_event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  kernel_launch_event_pool_desc.version = ZE_EVENT_POOL_DESC_VERSION_CURRENT;

  result = zeEventPoolCreate(context.driver, &kernel_launch_event_pool_desc, 1,
                             &context.device, kernel_launch_event_pool);
  if (result) {
    throw std::runtime_error("zeEventPoolCreate failed: " +
                             std::to_string(result));
  }
}

void single_event_create(ze_event_pool_handle_t event_pool,
                         ze_event_handle_t *event) {
  ze_result_t result;
  ze_event_desc_t event_desc = {};

  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_NONE;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_NONE;
  event_desc.version = ZE_EVENT_DESC_VERSION_CURRENT;
  result = zeEventCreate(event_pool, &event_desc, event);
  if (result) {
    throw std::runtime_error("zeEventCreate failed: " + std::to_string(result));
  }
}
//---------------------------------------------------------------------
// Utility function to execute a kernel function for a set of iterations
// and measure the time elapsed based off the timing type.
// This function takes a pre-calculated workgroup distribution
// and will time the kernel executed given the timing type.
// The current timing types supported are:
//          BANDWIDTH -> Average time to execute the kernel for # iterations
//          KERNEL_LAUNCH_LATENCY -> Average time to execute the kernel on
//                                  the command list
//          KERNEL_COMPLETE_LATENCY - Average time to execute a given kernel
//                                  for # iterations.
// On success, the average time is returned.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
long double ZePeak::run_kernel(L0Context context, ze_kernel_handle_t &function,
                               struct ZeWorkGroups &workgroup_info,
                               TimingMeasurement type,
                               bool reset_command_list) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  long double timed = 0;

  result = zeKernelSetGroupSize(function, workgroup_info.group_size_x,
                                workgroup_info.group_size_y,
                                workgroup_info.group_size_z);
  if (result) {
    throw std::runtime_error("zeKernelSetGroupSize failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Group size set\n";

  Timer timer;

  if (type == TimingMeasurement::BANDWIDTH) {
    result = zeCommandListAppendLaunchKernel(
        context.command_list, function, &workgroup_info.thread_group_dimensions,
        nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Function launch appended\n";

    result = zeCommandListClose(context.command_list);
    if (result) {
      throw std::runtime_error("zeCommandListClose failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Command list closed\n";

    for (uint32_t i = 0; i < warmup_iterations; i++) {
      run_command_queue(context);
    }

    synchronize_command_queue(context);

    timer.start();
    for (uint32_t i = 0; i < iters; i++) {
      run_command_queue(context);
    }
    synchronize_command_queue(context);
    timed = timer.stopAndTime();
  } else if (type == TimingMeasurement::BANDWIDTH_EVENT_TIMING) {
    ze_event_pool_handle_t event_pool;
    ze_event_handle_t function_event;

    single_event_pool_create(context, &event_pool);
    if (verbose)
      std::cout << "Event Pool Created\n";

    single_event_create(event_pool, &function_event);
    if (verbose)
      std::cout << "Event Created\n";

    result = zeCommandListAppendLaunchKernel(
        context.command_list, function, &workgroup_info.thread_group_dimensions,
        function_event, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Function launch appended\n";

    result = zeCommandListClose(context.command_list);
    if (result) {
      throw std::runtime_error("zeCommandListClose failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Command list closed\n";

    for (uint32_t i = 0; i < warmup_iterations; i++) {
      result = zeCommandQueueExecuteCommandLists(
          context.command_queue, 1, &context.command_list, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandQueueExecuteCommandLists failed: " +
                                 std::to_string(result));
      }

      result = zeEventHostSynchronize(function_event, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeEventHostSynchronize failed: " +
                                 std::to_string(result));
      }

      result = zeCommandQueueSynchronize(context.command_queue, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                                 std::to_string(result));
      }

      result = zeEventHostReset(function_event);
      if (result) {
        throw std::runtime_error("zeEventHostReset failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Event Reset" << std::endl;
    }

    for (uint32_t i = 0; i < iters; i++) {
      timer.start();
      result = zeCommandQueueExecuteCommandLists(
          context.command_queue, 1, &context.command_list, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandQueueExecuteCommandLists failed: " +
                                 std::to_string(result));
      }

      result = zeEventHostSynchronize(function_event, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeEventHostSynchronize failed: " +
                                 std::to_string(result));
      }
      timed += timer.stopAndTime();

      result = zeCommandQueueSynchronize(context.command_queue, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Command queue synchronized\n";

      result = zeEventHostReset(function_event);
      if (result) {
        throw std::runtime_error("zeEventHostReset failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Event Reset\n";
    }
    zeEventDestroy(function_event);
  } else if (type == TimingMeasurement::KERNEL_LAUNCH_LATENCY) {
    ze_event_handle_t kernel_launch_event;
    ze_event_pool_handle_t kernel_launch_event_pool;

    single_event_pool_create(context, &kernel_launch_event_pool);
    if (verbose)
      std::cout << "Event Pool Created\n";

    single_event_create(kernel_launch_event_pool, &kernel_launch_event);
    if (verbose)
      std::cout << "Event Created\n";
    result = zeCommandListAppendSignalEvent(context.command_list,
                                            kernel_launch_event);
    if (result) {
      throw std::runtime_error("zeCommandListAppendSignalEvent failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Kernel Launch Event signal appended to command list\n";

    result = zeCommandListAppendLaunchKernel(
        context.command_list, function, &workgroup_info.thread_group_dimensions,
        nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Function launch appended\n";

    result = zeCommandListClose(context.command_list);
    if (result) {
      throw std::runtime_error("zeCommandListClose failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Command list closed\n";

    for (uint32_t i = 0; i < warmup_iterations; i++) {
      run_command_queue(context);
      synchronize_command_queue(context);
      result = zeEventHostSynchronize(kernel_launch_event, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeEventHostSynchronize failed: " +
                                 std::to_string(result));
      }
      result = zeEventHostReset(kernel_launch_event);
      if (result) {
        throw std::runtime_error("zeEventHostReset failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Event Reset\n";
    }

    for (uint32_t i = 0; i < iters; i++) {
      timer.start();
      result = zeCommandQueueExecuteCommandLists(
          context.command_queue, 1, &context.command_list, nullptr);
      if (result) {
        throw std::runtime_error("zeCommandQueueExecuteCommandLists failed: " +
                                 std::to_string(result));
      }

      result = zeEventHostSynchronize(kernel_launch_event, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeEventHostSynchronize failed: " +
                                 std::to_string(result));
      }
      timed += timer.stopAndTime();

      result = zeCommandQueueSynchronize(context.command_queue, UINT32_MAX);
      if (result) {
        throw std::runtime_error("zeCommandQueueSynchronize failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Command queue synchronized\n";

      result = zeEventHostReset(kernel_launch_event);
      if (result) {
        throw std::runtime_error("zeEventHostReset failed: " +
                                 std::to_string(result));
      }
      if (verbose)
        std::cout << "Event Reset\n";
    }
  } else if (type == TimingMeasurement::KERNEL_COMPLETE_RUNTIME) {
    result = zeCommandListAppendLaunchKernel(
        context.command_list, function, &workgroup_info.thread_group_dimensions,
        nullptr, 0, nullptr);
    if (result) {
      throw std::runtime_error("zeCommandListAppendLaunchKernel failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Function launch appended\n";

    result = zeCommandListClose(context.command_list);
    if (result) {
      throw std::runtime_error("zeCommandListClose failed: " +
                               std::to_string(result));
    }
    if (verbose)
      std::cout << "Command list closed\n";

    for (uint32_t i = 0; i < warmup_iterations; i++) {
      run_command_queue(context);
    }

    synchronize_command_queue(context);

    for (uint32_t i = 0; i < iters; i++) {
      timer.start();
      run_command_queue(context);
      synchronize_command_queue(context);
      timed += timer.stopAndTime();
    }
  }

  if (reset_command_list)
    context.reset_commandlist();

  return (timed / static_cast<long double>(iters));
}

//---------------------------------------------------------------------
// Utility function to setup a kernel function with an input & output argument.
// On error, an exception will be thrown describing the failure.
//---------------------------------------------------------------------
void ZePeak::setup_function(L0Context &context, ze_kernel_handle_t &function,
                            const char *name, void *input, void *output,
                            size_t outputSize) {
  ze_kernel_desc_t function_description = {};
  ze_result_t result = ZE_RESULT_SUCCESS;

  function_description.version = ZE_KERNEL_DESC_VERSION_CURRENT;
  function_description.flags = ZE_KERNEL_FLAG_NONE;
  function_description.pKernelName = name;

  result = zeKernelCreate(context.module, &function_description, &function);
  if (result) {
    throw std::runtime_error("zeModuleCreateFunction failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Function created\n";

  result = zeKernelSetArgumentValue(function, 0, sizeof(input), &input);
  if (result) {
    throw std::runtime_error("zeKernelSetArgumentValue failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Input buffer set as function argument\n";

  // some kernels require scalar to be used on argument 1
  if (outputSize) {
    result = zeKernelSetArgumentValue(function, 1, outputSize, output);
  } else {
    result = zeKernelSetArgumentValue(function, 1, sizeof(output), &output);
  }

  if (result) {
    throw std::runtime_error("zeKernelSetArgumentValue failed: " +
                             std::to_string(result));
  }
  if (verbose)
    std::cout << "Output buffer set as function argument\n";
}

//---------------------------------------------------------------------
// Utility function to calculate the max work items that the current
// device can execute simultaneously with a single kernel function enqueued.
//---------------------------------------------------------------------
uint64_t ZePeak::get_max_work_items(L0Context &context) {
  return context.device_property.numSlices *
         context.device_property.numSubslicesPerSlice *
         context.device_property.numEUsPerSubslice *
         context.device_compute_property.maxGroupSizeX;
}

//---------------------------------------------------------------------
// Utility function to print a standard string to end a test.
//---------------------------------------------------------------------
void ZePeak::print_test_complete() {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
}

//---------------------------------------------------------------------
// Main function which calls the argument parsing and calls each
// test requested.
//---------------------------------------------------------------------
int main(int argc, char **argv) {
  ZePeak peak_benchmark;
  L0Context context;

  peak_benchmark.parse_arguments(argc, argv);
  context.verbose = peak_benchmark.verbose;

  context.init_xe();

  if (peak_benchmark.run_global_bw)
    peak_benchmark.ze_peak_global_bw(context);

  if (peak_benchmark.run_hp_compute)
    peak_benchmark.ze_peak_hp_compute(context);

  if (peak_benchmark.run_sp_compute)
    peak_benchmark.ze_peak_sp_compute(context);

  if (peak_benchmark.run_dp_compute)
    peak_benchmark.ze_peak_dp_compute(context);

  if (peak_benchmark.run_int_compute)
    peak_benchmark.ze_peak_int_compute(context);

  if (peak_benchmark.run_transfer_bw)
    peak_benchmark.ze_peak_transfer_bw(context);

  if (peak_benchmark.run_kernel_lat)
    peak_benchmark.ze_peak_kernel_latency(context);

  context.clean_xe();

  std::cout << std::flush;

  return 0;
}

#if defined(unix) || defined(__unix__) || defined(__unix)

#include <unistd.h>

unsigned long long int total_available_memory() {
  const long page_count = sysconf(_SC_PHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  const unsigned long long total_bytes = page_count * page_size;

  return total_bytes;
}

#endif

#if defined(_WIN64) || defined(_WIN64) || defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

unsigned long long int total_available_memory() {
  MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  GlobalMemoryStatusEx(&stat);

  return stat.ullAvailVirtual;
}

#endif

inline bool
is_integrated_gpu(ze_device_memory_properties_t &device_memory_properties) {
  return (device_memory_properties.totalSize == 0);
}

uint64_t max_device_object_size(L0Context &context) {
  ze_result_t result;

  ze_device_memory_properties_t device_memory_properties = {
      ZE_DEVICE_MEMORY_PROPERTIES_VERSION_CURRENT};

  uint32_t device_count = 1;
  result = zeDeviceGetMemoryProperties(context.device, &device_count,
                                       &device_memory_properties);
  if (result) {
    throw std::runtime_error("zeDeviceGetMemoryProperties failed: " +
                             std::to_string(result));
  }

  if (is_integrated_gpu(device_memory_properties)) {
    unsigned long long int total_memory = total_available_memory();
    if (total_memory > FOUR_GB) {
      return FOUR_GB - EIGHT_KB;
    } else {
      return std::max(total_memory / 4, 128 * ONE_MB);
    }
  } else {
    return device_memory_properties.totalSize;
  }
}

TimingMeasurement ZePeak::is_bandwidth_with_event_timer(void) {
  if (use_event_timer) {
    return TimingMeasurement::BANDWIDTH_EVENT_TIMING;
  } else {
    return TimingMeasurement::BANDWIDTH;
  }
}

long double ZePeak::calculate_gbps(long double period,
                                   long double buffer_size) {
  return buffer_size / period / 1e3f;
}
