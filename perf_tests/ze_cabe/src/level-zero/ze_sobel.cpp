/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_sobel.hpp"

namespace compute_api_bench {

ZeSobel::ZeSobel(level_zero_tests::ImageBMP8Bit image,
                 unsigned int num_iterations)
    : ZeWorkload(), num_iterations(num_iterations) {
  workload_name = "Sobel";
  width = image.width();
  height = image.height();
  image_buffer_size = width * height * sizeof(uint32_t);
  lena_original.assign(image_buffer_size, 0);
  lena_filtered_GPU.assign(image_buffer_size, 0);
  lena_filtered_CPU.assign(image_buffer_size, 0);

  for (unsigned int i = 0; i < width; i++) {
    for (unsigned int j = 0; j < height; j++) {
      lena_original[i + j * width] = (uint32_t)image.get_pixel(i, j);
    }
  }

  sobel_cpu(lena_original.data(), lena_filtered_CPU.data(), width, height);
  kernel_spv = load_binary_file(kernel_length, "ze_cabe_sobel.spv");
}

ZeSobel::~ZeSobel() {}

void ZeSobel::build_program() {
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = kernel_length;
  module_description.pInputModule = kernel_spv.data();
  module_description.pBuildFlags = nullptr;
  ZE_CHECK_RESULT(
      zeModuleCreate(context, device, &module_description, &module, nullptr));

  ze_kernel_desc_t function_description = {};
  function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
  function_description.pNext = nullptr;
  function_description.flags = 0;
  function_description.pKernelName = "sobel";
  ZE_CHECK_RESULT(zeKernelCreate(module, &function_description, &function));
}

void ZeSobel::create_buffers() {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, image_buffer_size, 1,
                                   device, &input_buffer));
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc, image_buffer_size, 1,
                                   device, &output_buffer));
}

void ZeSobel::create_cmdlist() {
  uint32_t group_size_x = 16;
  uint32_t group_size_y = 16;
  uint32_t group_size_z = 1;

  ZE_CHECK_RESULT(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 0, sizeof(input_buffer),
                                           &input_buffer));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 1, sizeof(output_buffer),
                                           &output_buffer));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 2, sizeof(int), &width));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 3, sizeof(int), &height));

  ze_command_list_desc_t command_list_description = {};
  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.pNext = nullptr;
  ZE_CHECK_RESULT(zeCommandListCreate(
      context, device, &command_list_description, &command_list));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, input_buffer, lena_original.data(), image_buffer_size,
      nullptr, 0, nullptr));
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));

  ze_group_count_t group_count;
  group_count.groupCountX = width / group_size_x;
  group_count.groupCountY = height / group_size_y;
  group_count.groupCountZ = 1;
  for (unsigned int i = 0; i < num_iterations; ++i) {
    ZE_CHECK_RESULT(zeCommandListAppendLaunchKernel(
        command_list, function, &group_count, nullptr, 0, nullptr));
  }
  ZE_CHECK_RESULT(
      zeCommandListAppendBarrier(command_list, nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListAppendMemoryCopy(
      command_list, lena_filtered_GPU.data(), output_buffer, image_buffer_size,
      nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListClose(command_list));

  ze_command_queue_desc_t command_queue_description = {};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = 0;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
  ZE_CHECK_RESULT(zeCommandQueueCreate(
      context, device, &command_queue_description, &command_queue));
}

void ZeSobel::execute_work() {
  ZE_CHECK_RESULT(zeCommandQueueExecuteCommandLists(command_queue, 1,
                                                    &command_list, nullptr));
  ZE_CHECK_RESULT(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
}

bool ZeSobel::verify_results() {
  for (unsigned int i = 0; i < width * height; i++) {
    if (lena_filtered_GPU[i] != lena_filtered_CPU[i]) {
      printf("\nGPU %d vs. CPU %d\n", lena_filtered_GPU[i],
             lena_filtered_CPU[i]);
      return false;
    }
  }

  return true;
}

void ZeSobel::cleanup() {
  ZE_CHECK_RESULT(zeCommandQueueDestroy(command_queue));
  ZE_CHECK_RESULT(zeCommandListDestroy(command_list));
  ZE_CHECK_RESULT(zeKernelDestroy(function));
  ZE_CHECK_RESULT(zeModuleDestroy(module));
  ZE_CHECK_RESULT(zeMemFree(context, output_buffer));
  ZE_CHECK_RESULT(zeMemFree(context, input_buffer));
  ZE_CHECK_RESULT(zeContextDestroy(context));
}

} // namespace compute_api_bench
