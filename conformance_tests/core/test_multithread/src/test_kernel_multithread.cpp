/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "random/random.hpp"
#include <thread>
#include <array>

#include <level_zero/ze_api.h>

namespace {

const int num_threads = 16;
const int num_iterations = 2000;
const uint32_t pattern_memory_count = 64;
const uint32_t pattern_memory_size = pattern_memory_count * sizeof(uint64_t);
const uint32_t output_count = pattern_memory_count;
const uint32_t output_size = pattern_memory_size;
void run_functions(lzt::zeCommandBundle &, bool, ze_kernel_handle_t,
                   ze_kernel_handle_t, void *, uint16_t, uint64_t *, uint64_t *,
                   uint64_t *, uint64_t *);

void thread_kernel_create_destroy(ze_module_handle_t module_handle,
                                  int thread_id, bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  ze_kernel_flags_t flag = 0;
  /* Prepare the fill function */
  ze_kernel_handle_t fill_function =
      lzt::create_function(module_handle, flag, "fill_device_memory");
  /* Prepare the test function */
  ze_kernel_handle_t test_function =
      lzt::create_function(module_handle, flag, "test_device_memory");

  // Create pattern buffer on which device will perform writes using the
  // compute kernel, create found and expected/gold buffers which are used to
  // validate the data written by device
  auto gpu_pattern_buffer =
      (uint64_t *)lzt::allocate_shared_memory(pattern_memory_size, 8);
  auto gpu_expected_output_buffer =
      (uint64_t *)lzt::allocate_device_memory(output_size, 8);
  auto gpu_found_output_buffer =
      (uint64_t *)lzt::allocate_device_memory(output_size, 8);

  uint64_t *host_expected_output_buffer = nullptr;
  uint64_t *host_found_output_buffer = nullptr;

  auto mem_access_properties = lzt::get_memory_access_properties(
      lzt::get_default_device(lzt::get_default_driver()));
  if ((mem_access_properties.sharedSystemAllocCapabilities &
       ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
    host_expected_output_buffer =
        static_cast<uint64_t *>(lzt::allocate_host_memory(output_size));
    host_found_output_buffer =
        static_cast<uint64_t *>(lzt::allocate_host_memory(output_size));
  } else {
    host_expected_output_buffer = new uint64_t[output_count]();
    host_found_output_buffer = new uint64_t[output_count]();
  }

  for (int i = 0; i < num_iterations; i++) {
    std::fill(host_expected_output_buffer,
              host_expected_output_buffer + output_count, 0);
    std::fill(host_found_output_buffer, host_found_output_buffer + output_count,
              0);
    // Access to pattern buffer from host.
    std::fill(gpu_pattern_buffer, gpu_pattern_buffer + pattern_memory_count, 0);
    uint16_t pattern_base = lzt::generate_value<uint16_t>();
    run_functions(cmd_bundle, is_immediate, fill_function, test_function,
                  gpu_pattern_buffer, pattern_base, host_expected_output_buffer,
                  gpu_expected_output_buffer, host_found_output_buffer,
                  gpu_found_output_buffer);

    for (uint32_t j = 0; j < output_count; j++) {
      if (host_expected_output_buffer[j] || host_found_output_buffer[j]) {
        FAIL() << "Test buffer mismatch on thread " << thread_id
               << " iteration " << i << ". Index = " << j
               << ", found = " << host_found_output_buffer[j]
               << ", expected = " << host_expected_output_buffer[j]
               << ", real = " << ((j << sizeof(uint16_t) * 8) + pattern_base);
        break;
      }
    }

    // Access to pattern buffer from host
    for (uint32_t j = 0; j < pattern_memory_count; j++) {
      if (gpu_pattern_buffer[j] !=
          ((j << (sizeof(uint16_t) * 8)) + pattern_base)) {
        FAIL() << "GPU buffer mismatch on thread " << thread_id << " iteration "
               << i << ". Index = " << j
               << ", buffer = " << gpu_pattern_buffer[j]
               << ", real = " << ((j << (sizeof(uint16_t) * 8)) + pattern_base);
        break;
      }
    }
  }

  if ((mem_access_properties.sharedSystemAllocCapabilities &
       ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
    lzt::free_memory(host_expected_output_buffer);
    lzt::free_memory(host_found_output_buffer);
  } else {
    delete[] host_expected_output_buffer;
    delete[] host_found_output_buffer;
  }
  lzt::free_memory(gpu_pattern_buffer);
  lzt::free_memory(gpu_expected_output_buffer);
  lzt::free_memory(gpu_found_output_buffer);
  lzt::destroy_function(fill_function);
  lzt::destroy_function(test_function);
  lzt::destroy_command_bundle(cmd_bundle);
}

void run_functions(lzt::zeCommandBundle &cmd_bundle, bool is_immediate,
                   ze_kernel_handle_t fill_function,
                   ze_kernel_handle_t test_function, void *pattern_memory,
                   uint16_t pattern_base, uint64_t *host_expected_output_buffer,
                   uint64_t *gpu_expected_output_buffer,
                   uint64_t *host_found_output_buffer,
                   uint64_t *gpu_found_output_buffer) {

  // Set the thread group size to make sure all the device threads are
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

  lzt::set_argument_value(fill_function, 1, sizeof(pattern_base),
                          &pattern_base);

  lzt::set_group_size(test_function, groupSizeX, groupSizeY, groupSizeZ);

  lzt::set_argument_value(test_function, 0, sizeof(pattern_memory),
                          &pattern_memory);

  lzt::set_argument_value(test_function, 1, sizeof(pattern_base),
                          &pattern_base);

  lzt::set_argument_value(test_function, 2, sizeof(gpu_expected_output_buffer),
                          &gpu_expected_output_buffer);

  lzt::set_argument_value(test_function, 3, sizeof(gpu_found_output_buffer),
                          &gpu_found_output_buffer);

  lzt::set_argument_value(test_function, 4, sizeof(output_count),
                          &output_count);

  // If groupSize is greater than memory count, then at least one thread group
  // should be dispatched
  uint32_t threadGroup = pattern_memory_count / groupSize > 1
                             ? pattern_memory_count / groupSize
                             : 1;
  LOG_DEBUG << "thread group dimension is ::" << threadGroup;
  ze_group_count_t thread_group_dimensions = {threadGroup, 1, 1};

  lzt::append_memory_copy(cmd_bundle.list, gpu_expected_output_buffer,
                          host_expected_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);
  lzt::append_memory_copy(cmd_bundle.list, gpu_found_output_buffer,
                          host_found_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);

  // Access to pattern buffer from device using the compute kernel to fill
  // data.
  lzt::append_launch_function(cmd_bundle.list, fill_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  // Access to pattern buffer from device using the compute kernel to test
  // data.
  lzt::append_launch_function(cmd_bundle.list, test_function,
                              &thread_group_dimensions, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);

  lzt::append_memory_copy(cmd_bundle.list, host_expected_output_buffer,
                          gpu_expected_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);
  lzt::append_memory_copy(cmd_bundle.list, host_found_output_buffer,
                          gpu_found_output_buffer,
                          output_count * sizeof(uint64_t), nullptr);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::reset_command_list(cmd_bundle.list);
}

void thread_module_create_destroy() {
  for (int i = 0; i < num_iterations; i++) {
    auto driver = lzt::get_default_driver();
    auto device = lzt::get_default_device(driver);
    ze_module_handle_t module_handle =
        lzt::create_module(device, "test_fill_device_memory_multi_thread.spv");
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));
  }
}

class zeKernelSubmissionMultithreadTest : public ::testing::Test {};

TEST_F(
    zeKernelSubmissionMultithreadTest,
    GivenMultipleThreadsWhenPerformingModuleCreateDestroyThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (int i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_module_create_destroy);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST_F(
    zeKernelSubmissionMultithreadTest,
    GivenMultipleThreadsWhenPerformingKernelSubmissionsThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module_handle =
      lzt::create_module(device, "test_fill_device_memory_multi_thread.spv");

  for (int i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_kernel_create_destroy,
                                               module_handle, i, false);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));
}

TEST_F(
    zeKernelSubmissionMultithreadTest,
    GivenMultipleThreadsWhenPerformingKernelSubmissionsOnImmediateCmdListThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;
  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_module_handle_t module_handle =
      lzt::create_module(device, "test_fill_device_memory_multi_thread.spv");

  for (int i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_kernel_create_destroy,
                                               module_handle, i, true);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(module_handle));
}

} // namespace
