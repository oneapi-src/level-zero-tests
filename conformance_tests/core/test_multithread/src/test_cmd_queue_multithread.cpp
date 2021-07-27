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
#include <vector>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

constexpr uint32_t thread_iters = 2000;
constexpr size_t num_threads = 16;

void thread_create_destroy_command_queue_default() {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();
    lzt::destroy_command_queue(cmd_queue);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void thread_create_destroy_command_queue_random_flags(
    ze_device_handle_t device, ze_command_queue_flag_t flags,
    ze_command_queue_mode_t mode, ze_command_queue_priority_t priority,
    uint32_t ordinal) {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  for (uint32_t i = 0; i < thread_iters; i++) {
    for (uint32_t j = 0; j < ordinal; j++) {
      ze_command_queue_handle_t cmd_queue =
          lzt::create_command_queue(device, flags, mode, priority, j);
      lzt::destroy_command_queue(cmd_queue);
    }
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void thread_create_destroy_max_command_queue(size_t max_cmd_queues) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  size_t num_cmd_queues_per_thread = max_cmd_queues / num_threads;
  std::vector<ze_command_queue_handle_t> cmd_queue;
  cmd_queue.resize(num_cmd_queues_per_thread);

  for (uint32_t i = 0; i < thread_iters; i++) {
    for (uint32_t i = 0; i < num_cmd_queues_per_thread; i++) {
      ze_command_queue_handle_t queue_handle = lzt::create_command_queue();
      cmd_queue.push_back(queue_handle);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    for (uint32_t i = 0; i < num_cmd_queues_per_thread; i++) {
      ze_command_queue_handle_t queue_handle = cmd_queue.back();
      lzt::destroy_command_queue(queue_handle);
      cmd_queue.pop_back();
    }
  }
  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeCommandQueueCreateDestroyDefaultMultithreadTest
    : public ::testing::Test {};

TEST(zeCommandQueueCreateDestroyDefaultMultithreadTest,
     GivenMultipleThreadsWhenCreateAndDestroyCommandQueueThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        thread_create_destroy_command_queue_default);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

class zeCommandQueueCreateDestroyRandomMultithreadTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flag_t, ze_command_queue_mode_t,
                     ze_command_queue_priority_t>> {};

TEST_P(
    zeCommandQueueCreateDestroyRandomMultithreadTest,
    GivenMultipleThreadsWhenCreateAndDestroyCommandQueueWithRandomValuesThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  ze_command_queue_flag_t flags = std::get<0>(GetParam());        // flags
  ze_command_queue_mode_t mode = std::get<1>(GetParam());         // mode
  ze_command_queue_priority_t priority = std::get<2>(GetParam()); // priority
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_device_properties_t properties = {};
  properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));

  uint32_t ordinal = 0;

  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device);

  if (command_queue_group_properties.size() == 0) {
    LOG_WARNING << "Not enough command queues to run test";
    SUCCEED();
    return;
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        thread_create_destroy_command_queue_random_flags, device, flags, mode,
        priority, ordinal);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

INSTANTIATE_TEST_CASE_P(
    TestAllInputPermuations, zeCommandQueueCreateDestroyRandomMultithreadTest,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH)));

class zeCreateDestroyMaxCommandQueueMultithreadTest : public ::testing::Test {};

TEST(
    zeMaxCommandQueueCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenCreateAndDestroyMaxCommandQueuesThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  auto properties = lzt::get_command_queue_group_properties(device);
  // TODO: adapt for different groups
  ASSERT_GT(properties.size(), 0);
  size_t max_cmd_queues = static_cast<size_t>(properties[0].numQueues);

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        thread_create_destroy_max_command_queue, max_cmd_queues);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

} // namespace
