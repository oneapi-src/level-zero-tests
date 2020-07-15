/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_simpleadd.hpp"

namespace compute_api_bench {

ZeSimpleAdd::ZeSimpleAdd(unsigned int num_elements)
    : ZeWorkload(), num(num_elements) {
  workload_name = "SimpleAdd";
  x.assign(num_elements, 1);
  y.assign(num_elements, 0);
  kernel_spv = load_binary_file(kernel_length, "ze_cabe_simpleadd.spv");
}

ZeSimpleAdd::~ZeSimpleAdd() {}

void ZeSimpleAdd::build_program() {
  ze_module_desc_t module_description = {};
  module_description.version = ZE_MODULE_DESC_VERSION_CURRENT;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = kernel_length;
  module_description.pInputModule = kernel_spv.data();
  module_description.pBuildFlags = nullptr;
  ZE_CHECK_RESULT(
      zeModuleCreate(device, &module_description, &module, nullptr));

  ze_kernel_desc_t function_description = {};
  function_description.version = ZE_KERNEL_DESC_VERSION_CURRENT;
  function_description.flags = ZE_KERNEL_FLAG_NONE;
  function_description.pKernelName = "NaiveAdd";
  ZE_CHECK_RESULT(zeKernelCreate(module, &function_description, &function));
}

void ZeSimpleAdd::create_buffers() {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.ordinal = 0;
  device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT;
  ZE_CHECK_RESULT(zeDriverAllocDeviceMem(driver_handle, &device_desc,
                                         sizeof(int) * num, 1, device,
                                         &input_buffer));
  ZE_CHECK_RESULT(zeDriverAllocDeviceMem(driver_handle, &device_desc,
                                         sizeof(int) * num, 1, device,
                                         &output_buffer));
}

void ZeSimpleAdd::create_cmdlist() {
  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  ZE_CHECK_RESULT(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));

  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 0, sizeof(int), &num));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 1, sizeof(input_buffer),
                                           &input_buffer));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 2, sizeof(output_buffer),
                                           &output_buffer));

  ze_command_list_desc_t command_list_description = {};
  command_list_description.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;
  ZE_CHECK_RESULT(
      zeCommandListCreate(device, &command_list_description, &command_list));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, input_buffer, x.data(), sizeof(int) * num, nullptr));
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;
  ZE_CHECK_RESULT(zeCommandListAppendLaunchKernel(
      command_list, function, &group_count, nullptr, 0, nullptr));
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, y.data(), output_buffer, sizeof(int) * num, nullptr));
  ZE_CHECK_RESULT(zeCommandListClose(command_list));

  const uint32_t command_queue_id = 0;
  ze_command_queue_desc_t command_queue_description = {};
  command_queue_description.version = ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT;
  command_queue_description.ordinal = command_queue_id;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
  ZE_CHECK_RESULT(
      zeCommandQueueCreate(device, &command_queue_description, &command_queue));
}

void ZeSimpleAdd::execute_work() {
  ZE_CHECK_RESULT(zeCommandQueueExecuteCommandLists(command_queue, 1,
                                                    &command_list, nullptr));
  ZE_CHECK_RESULT(zeCommandQueueSynchronize(command_queue, UINT32_MAX));
}

bool ZeSimpleAdd::verify_results() {
  for (unsigned int i = 0; i < 10; ++i) {
    if (y[i] != 2) {
      return false;
    }
  }
  return true;
}

void ZeSimpleAdd::cleanup() {
  ZE_CHECK_RESULT(zeCommandQueueDestroy(command_queue));
  ZE_CHECK_RESULT(zeCommandListDestroy(command_list));
  ZE_CHECK_RESULT(zeKernelDestroy(function));
  ZE_CHECK_RESULT(zeModuleDestroy(module));
  ZE_CHECK_RESULT(zeDriverFreeMem(driver_handle, output_buffer));
  ZE_CHECK_RESULT(zeDriverFreeMem(driver_handle, input_buffer));
}

} // namespace compute_api_bench
