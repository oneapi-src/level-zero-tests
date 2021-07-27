/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_mandelbrot.hpp"

namespace compute_api_bench {

ZeMandelbrot::ZeMandelbrot(unsigned int width, unsigned int height,
                           unsigned int num_iterations)
    : ZeWorkload(), width(width), height(height),
      num_iterations(num_iterations) {

  workload_name = "Mandelbrot";
  result.assign(width * height, 0);
  result_CPU.assign(width * height, 0);
  mandelbrot_cpu(result_CPU.data(), width, height);
  kernel_spv = load_binary_file(kernel_length, "ze_cabe_mandelbrot.spv");
}

ZeMandelbrot::~ZeMandelbrot() {}

void ZeMandelbrot::build_program() {
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
  function_description.pKernelName = "mandelbrot";
  ZE_CHECK_RESULT(zeKernelCreate(module, &function_description, &function));
}

void ZeMandelbrot::create_buffers() {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = 0;
  device_desc.flags = 0;
  ZE_CHECK_RESULT(zeMemAllocDevice(context, &device_desc,
                                   sizeof(float) * width * height, 1, device,
                                   &output_buffer));
}

void ZeMandelbrot::create_cmdlist() {
  uint32_t group_size_x = 16;
  uint32_t group_size_y = 16;
  uint32_t group_size_z = 1;

  ZE_CHECK_RESULT(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 0, sizeof(output_buffer),
                                           &output_buffer));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 1, sizeof(int), &width));
  ZE_CHECK_RESULT(zeKernelSetArgumentValue(function, 2, sizeof(int), &width));

  ze_command_list_desc_t command_list_description = {};
  command_list_description.stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC;
  command_list_description.pNext = nullptr;
  ZE_CHECK_RESULT(zeCommandListCreate(
      context, device, &command_list_description, &command_list));

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
      command_list, result.data(), output_buffer,
      sizeof(float) * width * height, nullptr, 0, nullptr));
  ZE_CHECK_RESULT(zeCommandListClose(command_list));

  ze_command_queue_desc_t command_queue_description = {};
  command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  command_queue_description.pNext = nullptr;
  command_queue_description.ordinal = 0;
  command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
  ZE_CHECK_RESULT(zeCommandQueueCreate(
      context, device, &command_queue_description, &command_queue));
}

void ZeMandelbrot::execute_work() {
  ZE_CHECK_RESULT(zeCommandQueueExecuteCommandLists(command_queue, 1,
                                                    &command_list, nullptr));
  ZE_CHECK_RESULT(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
}

bool ZeMandelbrot::verify_results() {
  bool save_images_and_quit = false;

  // ToDo: Somehow the precision is sligtly different on CPU and GPU
  for (unsigned int i = 0; i < width / 3 * height / 3; i++) {
    if (result[i] - result_CPU[i] > 0) {
      printf("\nGPU %d vs. CPU %d\n", (unsigned char)(255.0f * (result[i])),
             (unsigned char)(255.0f * (result_CPU[i])));
      save_images_and_quit = true;
      break;
    }
  }

  if (save_images_and_quit) {
    printf("Saving CPU and GPU generated images on the disk.. \n");
    FILE *f1 = fopen("ze_mandelbrot_result.ppm", "w");
    if (f1 == nullptr) {
      printf("Failed to open ze_mandelbrot_result.ppm\n");
      exit(1);
    }
    fprintf(f1, "P3\n%d %d\n%d\n", width, height, 255);

    for (unsigned int i = 0; i < width * height; i++)
      fprintf(f1, "%d %d %d ", (unsigned char)(255.0f * (result[i])),
              (unsigned char)(255.0f * (result[i])),
              (unsigned char)(255.0f * (result[i])));
    fclose(f1);
    FILE *f2 = fopen("cpu_mandelbrot_result.ppm", "w");
    if (f2 == nullptr) {
      printf("Failed to open cpu_mandelbrot_result.ppm\n");
      exit(1);
    }
    fprintf(f2, "P3\n%d %d\n%d\n", width, height, 255);

    for (unsigned int i = 0; i < width * height; i++)
      fprintf(f2, "%d %d %d ", (unsigned char)(255.0f * (result_CPU[i])),
              (unsigned char)(255.0f * (result_CPU[i])),
              (unsigned char)(255.0f * (result_CPU[i])));

    fclose(f2);
    return false;
  }

  return true;
}

void ZeMandelbrot::cleanup() {
  ZE_CHECK_RESULT(zeCommandQueueDestroy(command_queue));
  ZE_CHECK_RESULT(zeCommandListDestroy(command_list));
  ZE_CHECK_RESULT(zeKernelDestroy(function));
  ZE_CHECK_RESULT(zeModuleDestroy(module));
  ZE_CHECK_RESULT(zeMemFree(context, output_buffer));
  ZE_CHECK_RESULT(zeContextDestroy(context));
}

} // namespace compute_api_bench
