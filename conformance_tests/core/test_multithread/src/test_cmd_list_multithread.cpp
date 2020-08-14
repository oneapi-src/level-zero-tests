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

constexpr uint32_t thread_iters = 2000;
constexpr size_t num_threads = 16;

void thread_create_destroy_command_list_default() {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ze_command_list_handle_t cmd_list = lzt::create_command_list();
    lzt::close_command_list(cmd_list);
    lzt::destroy_command_list(cmd_list);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void thread_create_destroy_command_list_valid_flags(
    ze_device_handle_t device, ze_command_list_flag_t flags) {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ze_command_list_handle_t cmd_list = lzt::create_command_list(device, flags);
    lzt::close_command_list(cmd_list);
    lzt::destroy_command_list(cmd_list);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeCommandListCreateDestroyDefaultMultithreadTest
    : public ::testing::Test {};

TEST(
    zeCommandListCreateDestroyDefaultMultithreadTest,
    GivenMultipleThreadsWhenCreateCloseAndDestroyCommandListsThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        thread_create_destroy_command_list_default);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

class zeCommandListCreateDestroyValidFlagsMultithreadTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_list_flag_t> {};

TEST_P(
    zeCommandListCreateDestroyValidFlagsMultithreadTest,
    GivenMultipleThreadsWhenCreateCloseAndDestroyCommandListWithValidFlagValuesThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  ze_command_list_flag_t flags = GetParam(); // flags

  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        thread_create_destroy_command_list_valid_flags, device, flags);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

INSTANTIATE_TEST_CASE_P(
    TestCasesforCommandListCreateCloseAndDestroyWithValidFlagValues,
    zeCommandListCreateDestroyValidFlagsMultithreadTest,
    testing::Values(ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                    ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                    ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY));

} // namespace
