/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "random/random.hpp"
#include "utils/utils.hpp"
#include <boost/compute/core.hpp>
#include <thread>
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;
namespace compute = boost::compute;

namespace {

class zeCLInteropTests : public ::testing::Test {
protected:
  zeCLInteropTests() {
    cl_context_ = compute::system::default_context();
    cl_command_queue_ = compute::system::default_queue();
    compute::device device = compute::system::default_device();
    cl_int ret;
    static char hostBufA[1024];
    cl_memory_ =
        clCreateBuffer(cl_context_, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                       sizeof(hostBufA), hostBufA, &ret);
    EXPECT_NE(nullptr, cl_memory_);
    EXPECT_EQ(0, ret);

    const char *KernelSource = {
        "\n"
        "__kernel void square( __global float* input, __global float* output, "
        "\n"
        " const unsigned int count) {            \n"
        " int i = get_global_id(0);              \n"
        " if(i < count) \n"
        " output[i] = input[i] * input[i] * input[i]; \n"
        "}                     \n"};
    cl_program_ = clCreateProgramWithSource(
        cl_context_, 1, (const char **)&KernelSource, NULL, &ret);
    EXPECT_NE(nullptr, cl_program_);
    EXPECT_EQ(0, ret);
    ret = clBuildProgram(cl_program_, 1, &device.get(), NULL, NULL, NULL);
    EXPECT_EQ(0, ret);
  }
  ~zeCLInteropTests() { clReleaseMemObject(cl_memory_); }
  compute::context cl_context_;
  compute::command_queue cl_command_queue_;
  cl_mem cl_memory_;
  cl_program cl_program_;
};

class zeDeviceRegisterCLCommandQueueTests : public zeCLInteropTests {};

TEST_F(
    zeDeviceRegisterCLCommandQueueTests,
    GivenOpenCLContextAndOpenCLCommandQueueWhenRegisteringOpenCLCommandQueueThenNotNullCommandQueueIsReturned) {
  ze_command_queue_handle_t command_queue = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceRegisterCLCommandQueue(
                lzt::zeDevice::get_instance()->get_device(), cl_context_.get(),
                cl_command_queue_.get(), &command_queue));
  EXPECT_NE(nullptr, command_queue);
}

class zeDeviceRegisterCLProgramTests : public zeCLInteropTests {};

TEST_F(
    zeDeviceRegisterCLProgramTests,
    GivenOpenCLContextAndOpenCLProgramWhenRegisteringOpenCLProgramThenNotNullModuleHandleIsReturned) {
  ze_module_handle_t module_handle = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceRegisterCLProgram(
                                   lzt::zeDevice::get_instance()->get_device(),
                                   cl_context_, cl_program_, &module_handle));
  EXPECT_NE(nullptr, module_handle);
}

class zeDeviceRegisterCLMemoryTests : public zeCLInteropTests {};

TEST_F(
    zeDeviceRegisterCLMemoryTests,
    GivenOpenCLContextAndCLDeviceMemoryWhenRegisteringCLMemoryThenNotNullMemoryIsReturned) {
  void *ptr = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceRegisterCLMemory(
                                   lzt::zeDevice::get_instance()->get_device(),
                                   cl_context_.get(), cl_memory_, &ptr));
  EXPECT_NE(nullptr, ptr);
}

static void bitonic_sort_kernel_cpu(std::vector<int> &a, int p, int q) {
  const int d = 1 << (p - q);

  for (unsigned i = 0; i < a.size(); i++) {
    if ((i & d) == 0) {
      const int up = ((i >> p) & 2) == 0;

      if ((a[i] > a[i | d]) == up) {
        const int t = a[i];
        a[i] = a[i | d];
        a[i | d] = t;
      }
    }
  }
}

static int log2(size_t n) {
  int log2n = 0;
  while (n >>= 1) {
    ++log2n;
  }
  return log2n;
}

static std::vector<int> bitonic_sort_cpu(std::vector<int> data) {
  for (int p = 0; p < log2(data.size()); p++) {
    for (int q = 0; q <= p; q++) {
      bitonic_sort_kernel_cpu(data, p, q);
    }
  }
  return data;
}

static std::vector<int> bitonic_sort_cl(std::vector<int> input) {
  cl_int ret, status;
  auto cl_context = compute::system::default_context();
  auto command_queue = compute::system::default_queue();
  auto cl_device = compute::system::default_device();

  const std::vector<uint8_t> binary_file =
      lzt::load_binary_file("bitonic_sort.spv");
  size_t size = binary_file.size();
  auto binary_data = binary_file.data();
  cl_program program =
      clCreateProgramWithBinary(cl_context.get(), 1, &cl_device.get(), &size,
                                &binary_data, &status, &ret);
  EXPECT_EQ(ret, 0);
  ret = clBuildProgram(program, 1, &cl_device.get(), NULL, NULL, NULL);
  EXPECT_EQ(ret, 0);

  cl_kernel kernel = clCreateKernel(program, "bitonic_sort", &ret);

  auto cl_buffer = clCreateBuffer(
      cl_context, 0, input.size() * sizeof(input[0]), nullptr, &ret);
  EXPECT_NE(nullptr, cl_buffer);
  EXPECT_EQ(ret, 0);

  // copy input to device buffer
  clEnqueueWriteBuffer(command_queue, cl_buffer, true, 0,
                       input.size() * sizeof(input[0]), input.data(), 0,
                       nullptr, nullptr);
  clEnqueueBarrierWithWaitList(command_queue, 0, nullptr, nullptr);

  // append all function executions
  clSetKernelArg(kernel, 0, sizeof(cl_buffer), &cl_buffer);
  for (int p = 0; p < log2(input.size()); ++p) {
    for (int q = 0; q <= p; ++q) {

      clSetKernelArg(kernel, 1, sizeof(p), &p);
      clSetKernelArg(kernel, 2, sizeof(q), &q);
      const size_t global_work_size = input.size();
      ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,
                                   &global_work_size, nullptr, 0, nullptr,
                                   nullptr);
      EXPECT_EQ(ret, 0);
      clEnqueueBarrierWithWaitList(command_queue, 0, nullptr, nullptr);
    }
  }

  // copy data back, execute, sync

  clEnqueueReadBuffer(command_queue, cl_buffer, true, 0,
                      input.size() * sizeof(input[0]), input.data(), 0, nullptr,
                      nullptr);
  clFlush(command_queue);

  clReleaseProgram(program);
  clReleaseMemObject(cl_buffer);
  return input;
}

static std::vector<int> bitonic_sort_l0(std::vector<int> input) {

  cl_int ret, status;
  auto cl_context = compute::system::default_context();
  auto cl_command_queue = compute::system::default_queue();
  auto cl_device = compute::system::default_device();

  const std::vector<uint8_t> binary_file =
      lzt::load_binary_file("bitonic_sort.spv");
  size_t size = binary_file.size();
  auto binary_data = binary_file.data();
  cl_program program =
      clCreateProgramWithBinary(cl_context.get(), 1, &cl_device.get(), &size,
                                &binary_data, &status, &ret);
  EXPECT_EQ(ret, 0);
  ret = clBuildProgram(program, 1, &cl_device.get(), NULL, NULL, NULL);
  EXPECT_EQ(ret, 0);

  auto cl_buffer = clCreateBuffer(
      cl_context, 0, input.size() * sizeof(input[0]), nullptr, &ret);
  EXPECT_NE(nullptr, cl_buffer);
  EXPECT_EQ(ret, 0);

  auto l0_mod = lzt::ocl_register_program(cl_context.get(), program);
  auto l0_buf = lzt::ocl_register_memory(cl_context.get(), cl_buffer);
  auto l0_cmd_queue =
      lzt::ocl_register_commandqueue(cl_context.get(), cl_command_queue.get());

  auto cmd_list = lzt::create_command_list();
  auto func = lzt::create_function(l0_mod, "bitonic_sort");

  // copy input to device buffer
  lzt::append_memory_copy(cmd_list, l0_buf, input.data(),
                          input.size() * sizeof(input[0]), nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);

  // append all function executions
  uint32_t group_size_x = 0;
  uint32_t group_size_y = 0;
  uint32_t group_size_z = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestGroupSize(func, input.size(), 1, 1, &group_size_x,
                                     &group_size_y, &group_size_z));

  lzt::set_group_size(func, group_size_x, group_size_y, group_size_z);
  lzt::set_argument_value(func, 0, sizeof(l0_buf), &l0_buf);

  for (int p = 0; p < log2(input.size()); ++p) {
    for (int q = 0; q <= p; ++q) {

      lzt::set_argument_value(func, 1, sizeof(p), &p);
      lzt::set_argument_value(func, 2, sizeof(q), &q);

      const int group_count_x = static_cast<int>(input.size()) / group_size_x;

      ze_group_count_t thread_group_dimensions;
      thread_group_dimensions.groupCountX = group_count_x;
      thread_group_dimensions.groupCountY = 1;
      thread_group_dimensions.groupCountZ = 1;

      zeCommandListAppendLaunchKernel(cmd_list, func, &thread_group_dimensions,
                                      nullptr, 0, nullptr);
      lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
    }
  }

  // copy data back, close, execute, sync
  lzt::append_memory_copy(cmd_list, input.data(), l0_buf,
                          input.size() * sizeof(input[0]), nullptr);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(l0_cmd_queue, 1, &cmd_list, nullptr);
  lzt::synchronize(l0_cmd_queue, UINT32_MAX);

  // cleanup
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(func);
  lzt::free_memory(l0_buf);
  lzt::destroy_command_queue(l0_cmd_queue);
  lzt::destroy_module(l0_mod);

  clReleaseProgram(program);
  clReleaseMemObject(cl_buffer);

  return input;
}

static bool is_power_of_two(const unsigned n) {
  return (n > 0) && (0 == (n & (n - 1)));
}

TEST(zeCLInteropTests,
     GivenOpenClStructsWhenSortingOnKernelThenCorrectResultIsOutput) {

  std::vector<int> input = lzt::generate_vector<int>(1 << 16, 0);
  EXPECT_TRUE(is_power_of_two(input.size()));

  auto output_cpu = bitonic_sort_cpu(input);
  EXPECT_TRUE(std::is_sorted(output_cpu.begin(), output_cpu.end()));
  auto output_l0 = bitonic_sort_l0(input);
  EXPECT_TRUE(std::is_sorted(output_l0.begin(), output_l0.end()));
  auto output_cl = bitonic_sort_cl(input);
  EXPECT_TRUE(std::is_sorted(output_cl.begin(), output_cl.end()));

  EXPECT_TRUE(output_cpu == output_l0);
  EXPECT_TRUE(output_cpu == output_cl);
}

TEST(
    zeCLInteropTests,
    GivenOCLAndL0KernelSortRoutinesWhenRunningBothConcurrentlyThenCorrectResultIsOutput) {

  std::vector<int> input = lzt::generate_vector<int>(1 << 16, 0);
  EXPECT_TRUE(is_power_of_two(input.size()));

  auto output_cpu = bitonic_sort_cpu(input);
  EXPECT_TRUE(std::is_sorted(output_cpu.begin(), output_cpu.end()));

  std::vector<int> output_cl;
  std::vector<int> output_l0;
  std::thread l0([&] { output_l0 = bitonic_sort_l0(input); });
  std::thread cl([&] { output_cl = bitonic_sort_cl(input); });

  l0.join();
  cl.join();

  EXPECT_TRUE(output_cpu == output_l0);
  EXPECT_TRUE(output_cpu == output_cl);
}

} // namespace
