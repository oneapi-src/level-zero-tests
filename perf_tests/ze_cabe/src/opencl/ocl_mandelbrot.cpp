/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ocl_mandelbrot.hpp"

namespace compute_api_bench {

OCLMandelbrot::OCLMandelbrot(unsigned int width, unsigned int height,
                             unsigned int num_iterations)
    : OCLWorkload(), width(width), height(height),
      num_iterations(num_iterations) {

  workload_name = "Mandelbrot";
  result.assign(width * height, 0);
  result_CPU.assign(width * height, 0);
  mandelbrot_cpu(result_CPU.data(), width, height);
  kernel_spv = load_binary_file(kernel_length, "ze_cabe_mandelbrot.spv");
}

OCLMandelbrot::~OCLMandelbrot() {}

void OCLMandelbrot::build_program() { prepare_program_from_binary(); }

void OCLMandelbrot::create_buffers() {
  memobjResult = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                sizeof(float) * width * height, NULL, &ret);
  CL_CHECK_RESULT(ret);
}

void OCLMandelbrot::create_cmdlist() {
  command_queue = clCreateCommandQueue(context, device_id, 0, &ret);
  CL_CHECK_RESULT(ret);
  kernel = clCreateKernel(program, "mandelbrot", &ret);
  CL_CHECK_RESULT(ret);
  CL_CHECK_RESULT(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&memobjResult));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 1, sizeof(int), &width));
  CL_CHECK_RESULT(clSetKernelArg(kernel, 2, sizeof(int), &height));
}

void OCLMandelbrot::execute_work() {
  size_t global_item_size[2] = {width, height};
  size_t local_item_size[2] = {16, 16};
  for (unsigned int i = 0; i < num_iterations; ++i) {
    CL_CHECK_RESULT(clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,
                                           global_item_size, local_item_size, 0,
                                           NULL, NULL));
  }
  CL_CHECK_RESULT(clEnqueueReadBuffer(command_queue, memobjResult, CL_TRUE, 0,
                                      sizeof(float) * width * height,
                                      result.data(), 0, NULL, NULL));
}

bool OCLMandelbrot::verify_results() {
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
    FILE *f1 = fopen("ocl_mandelbrot_result.ppm", "w");
    if (f1 == nullptr) {
      printf("Failed to open ocl_mandelbrot_result.ppm\n");
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

void OCLMandelbrot::cleanup() {
  CL_CHECK_RESULT(clFlush(command_queue));
  CL_CHECK_RESULT(clFinish(command_queue));
  CL_CHECK_RESULT(clReleaseKernel(kernel));
  CL_CHECK_RESULT(clReleaseProgram(program));
  CL_CHECK_RESULT(clReleaseMemObject(memobjResult));
  CL_CHECK_RESULT(clReleaseCommandQueue(command_queue));
  CL_CHECK_RESULT(clReleaseContext(context));
}

} // namespace compute_api_bench
