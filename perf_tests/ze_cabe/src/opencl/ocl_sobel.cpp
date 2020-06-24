/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ocl_sobel.hpp"

namespace compute_api_bench {

OCLSobel::OCLSobel(level_zero_tests::ImageBMP8Bit image,
                   unsigned int num_iterations)
    : OCLWorkload(), num_iterations(num_iterations) {
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

OCLSobel::~OCLSobel() {}

void OCLSobel::build_program() { prepare_program_from_binary(); }

void OCLSobel::create_buffers() {
  memobj_original =
      clCreateBuffer(context, CL_MEM_READ_WRITE, image_buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
  memobj_filtered =
      clCreateBuffer(context, CL_MEM_READ_WRITE, image_buffer_size, NULL, &ret);
  CL_CHECK_RESULT(ret);
}

void OCLSobel::create_cmdlist() {
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
  CL_CHECK_RESULT(ret);
  kernel = clCreateKernel(program, "sobel", &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memobj_original));
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&memobj_filtered));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 2, sizeof(int), &width));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 3, sizeof(int), &height));
}

void OCLSobel::execute_work() {
  CL_CHECK_RESULT(clEnqueueWriteBuffer(command_queue, memobj_original, CL_TRUE,
                                       0, image_buffer_size,
                                       lena_original.data(), 0, NULL, NULL));
  size_t global_item_size[2] = {width, height};
  size_t local_item_size[2] = {16, 16};
  for (unsigned int i = 0; i < num_iterations; ++i) {
    CL_CHECK_RESULT(clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,
                                           global_item_size, local_item_size, 0,
                                           NULL, NULL));
  }
  CL_CHECK_RESULT(clEnqueueReadBuffer(command_queue, memobj_filtered, CL_TRUE,
                                      0, image_buffer_size,
                                      lena_filtered_GPU.data(), 0, NULL, NULL));
}

bool OCLSobel::verify_results() {
  for (unsigned int i = 0; i < width * height; i++) {
    if (lena_filtered_GPU[i] != lena_filtered_CPU[i]) {
      printf("\nGPU %d vs. CPU %d\n", lena_filtered_GPU[i],
             lena_filtered_CPU[i]);
      return false;
    }
  }

  return true;
}

void OCLSobel::cleanup() {
  CL_CHECK_RESULT(clFlush(command_queue));
  CL_CHECK_RESULT(clFinish(command_queue));
  CL_CHECK_RESULT(clReleaseKernel(kernel));
  CL_CHECK_RESULT(clReleaseProgram(program));
  CL_CHECK_RESULT(clReleaseMemObject(memobj_original));
  CL_CHECK_RESULT(clReleaseMemObject(memobj_filtered));
  CL_CHECK_RESULT(clReleaseCommandQueue(command_queue));
  CL_CHECK_RESULT(clReleaseContext(context));
}

} // namespace compute_api_bench
