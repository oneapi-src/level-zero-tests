/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <thread>
#include <array>

namespace lzt = level_zero_tests;
#include <level_zero/ze_api.h>

namespace {

constexpr size_t num_threads = 16;
constexpr size_t num_iterations = 2000;
constexpr uint32_t output_count_ = 64;
constexpr size_t output_size_ = output_count_ * sizeof(uint64_t);
constexpr size_t pattern_memory_size = 512;
constexpr size_t pattern_memory_count =
    pattern_memory_size >> 3; // array of uint64_t
void run_functions(ze_command_queue_handle_t, ze_command_list_handle_t,
                   ze_kernel_handle_t, ze_kernel_handle_t, void *, size_t,
                   uint16_t, uint64_t *, uint64_t *, uint64_t *, uint64_t *,
                   size_t);

void thread_kernel_create_destroy(ze_module_handle_t module_handle) {
  ze_command_queue_handle_t command_queue = lzt::create_command_queue();
  ze_command_list_handle_t command_list = lzt::create_command_list();

  ze_kernel_flags_t flag = 0;
  /* Prepare the fill function */
  ze_kernel_handle_t fill_function =
      lzt::create_function(module_handle, flag, "fill_device_memory");
  /* Prepare the test function */
  ze_kernel_handle_t test_function =
      lzt::create_function(module_handle, flag, "test_device_memory");

  // create pattern buffer on which device  will perform writes using the
  // compute kernel, create found and expected/gold buffers which are used to
  // validate the data written by device
  uint64_t *gpu_pattern_buffer;
  gpu_pattern_buffer = (uint64_t *)level_zero_tests::allocate_shared_memory(
      pattern_memory_size, 8);
  uint64_t *gpu_expected_output_buffer;
  gpu_expected_output_buffer =
      (uint64_t *)level_zero_tests::allocate_device_memory(output_size_, 8);
  uint64_t *gpu_found_output_buffer;
  gpu_found_output_buffer =
      (uint64_t *)level_zero_tests::allocate_device_memory(output_size_, 8);
  uint64_t *host_expected_output_buffer = new uint64_t[output_count_];
  uint64_t *host_found_output_buffer = new uint64_t[output_size_];

  for (uint32_t i = 0; i < num_iterations; i++) {
    std::fill(host_expected_output_buffer,
              host_expected_output_buffer + output_count_, 0);
    std::fill(host_found_output_buffer,
              host_found_output_buffer + output_count_, 0);
    // Access to pattern buffer from host.
    std::fill(gpu_pattern_buffer, gpu_pattern_buffer + pattern_memory_count,
              0x0);
    uint16_t pattern_base = std::rand() % 0xFFFF;
    uint16_t pattern_base_1 = std::rand() % 0xFFFF;
    run_functions(command_queue, command_list, fill_function, test_function,
                  gpu_pattern_buffer, pattern_memory_count, pattern_base,
                  host_expected_output_buffer, gpu_expected_output_buffer,
                  host_found_output_buffer, gpu_found_output_buffer,
                  output_count_);

    bool memory_test_failure = false;
    for (uint32_t i = 0; i < output_count_; i++) {
      if (host_expected_output_buffer[i] || host_found_output_buffer[i]) {
        LOG_DEBUG << "Index of difference " << i << " found "
                  << host_found_output_buffer[i] << " expected "
                  << host_expected_output_buffer[i];
        memory_test_failure = true;
        break;
      }
    }

    EXPECT_EQ(false, memory_test_failure);

    // Access to pattern buffer from host
    bool host_test_failure = false;
    for (uint32_t i = 0; i < pattern_memory_count; i++) {
      if (gpu_pattern_buffer[i] !=
          ((i << (sizeof(uint16_t) * 8)) + pattern_base)) {
        LOG_DEBUG << "Index of difference " << i << " found "
                  << gpu_pattern_buffer[i] << " expected "
                  << ((i << (sizeof(uint16_t) * 8)) + pattern_base);
        host_test_failure = true;
        break;
      }
    }

    EXPECT_EQ(false, host_test_failure);
  }

  lzt::synchronize(command_queue, UINT64_MAX);

  delete[] host_expected_output_buffer;
  delete[] host_found_output_buffer;
  level_zero_tests::free_memory(gpu_pattern_buffer);
  level_zero_tests::free_memory(gpu_expected_output_buffer);
  level_zero_tests::free_memory(gpu_found_output_buffer);
  lzt::destroy_function(fill_function);
  lzt::destroy_function(test_function);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

void run_functions(ze_command_queue_handle_t command_queue,
                   ze_command_list_handle_t command_list,
                   ze_kernel_handle_t fill_function,
                   ze_kernel_handle_t test_function, void *pattern_memory,
                   size_t pattern_memory_count, uint16_t sub_pattern,
                   uint64_t *host_expected_output_buffer,
                   uint64_t *gpu_expected_output_buffer,
                   uint64_t *host_found_output_buffer,
                   uint64_t *gpu_found_output_buffer, size_t output_count) {

  // set the thread group size to make sure all the device threads are
  // occupied
  uint32_t groupSizeX, groupSizeY, groupSizeZ;
  size_t groupSize;
  lzt::suggest_group_size(fill_function, pattern_memory_count, 1, 1, groupSizeX,
                          groupSizeY, groupSizeZ);

  groupSize = groupSizeX * groupSizeY * groupSizeZ;
  LOG_DEBUG << "thread group size X is ::" << groupSizeX;
  LOG_DEBUG << "thread group size Y is ::" << groupSizeY;
  LOG_DEBUG << "thread group size Z is ::" << groupSizeZ;
  LOG_DEBUG << "thread group size is ::" << groupSize;
  lzt::set_group_size(fill_function, groupSizeX, groupSizeY, groupSizeZ);

  lzt::set_argument_value(fill_function, 0, sizeof(pattern_memory),
                          &pattern_memory);

  lzt::set_argument_value(fill_function, 1, sizeof(pattern_memory_count),
                          &pattern_memory_count);

  lzt::set_argument_value(fill_function, 2, sizeof(sub_pattern), &sub_pattern);

  lzt::set_group_size(test_function, groupSizeX, groupSizeY, groupSizeZ);

  lzt::set_argument_value(test_function, 0, sizeof(pattern_memory),
                          &pattern_memory);

  lzt::set_argument_value(test_function, 1, sizeof(pattern_memory_count),
                          &pattern_memory_count);

  lzt::set_argument_value(test_function, 2, sizeof(sub_pattern), &sub_pattern);

  lzt::set_argument_value(test_function, 3, sizeof(gpu_expected_output_buffer),
                          &gpu_expected_output_buffer);

  lzt::set_argument_value(test_function, 4, sizeof(gpu_found_output_buffer),
                          &gpu_found_output_buffer);

  lzt::set_argument_value(test_function, 5, sizeof(output_count),
                          &output_count);

  // if groupSize is greater then memory count, then at least one thread group
  // should be dispatched
  uint32_t threadGroup = pattern_memory_count / groupSize > 1
                             ? pattern_memory_count / groupSize
                             : 1;
  LOG_DEBUG << "thread group dimension is ::" << threadGroup;
  ze_group_count_t thread_group_dimensions = {threadGroup, 1, 1};

  lzt::append_memory_copy(command_list, gpu_expected_output_buffer,
                          host_expected_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);
  lzt::append_memory_copy(command_list, gpu_found_output_buffer,
                          host_found_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);

  // Access to pattern buffer from device using the compute kernel to fill
  // data.
  lzt::append_launch_function(command_list, fill_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  // Access to pattern buffer from device using the compute kernel to test
  // data.
  lzt::append_launch_function(command_list, test_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);

  lzt::append_memory_copy(command_list, host_expected_output_buffer,
                          gpu_expected_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);
  lzt::append_memory_copy(command_list, host_found_output_buffer,
                          gpu_found_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);
  lzt::close_command_list(command_list);

  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  lzt::reset_command_list(command_list);
}

void thread_module_create_destroy() {
  for (uint32_t i = 0; i < num_iterations; i++) {

    auto driver = lzt::get_default_driver();
    auto device = lzt::get_default_device(driver);
    ze_module_handle_t module_handle =
        lzt::create_module(device, "test_fill_device_memory.spv");
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));
  }
}

class zeKernelSubmissionMultithreadTest : public ::testing::Test {};

TEST(
    zeKernelSubmissionMultithreadTest,
    GivenMultipleThreadsWhenPerformingModuleCreateDestroyThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_module_create_destroy);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST(zeKernelSubmissionMultithreadTest,
     GivenMultipleThreadsWhenPerformingKernelSubmissionsThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module_handle =
      lzt::create_module(device, "test_fill_device_memory.spv");

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_kernel_create_destroy,
                                               module_handle);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));
}

} // namespace
