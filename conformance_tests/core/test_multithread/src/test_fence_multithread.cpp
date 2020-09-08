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
#include "test_harness/test_harness_fence.hpp"
#include "logging/logging.hpp"
#include <chrono>
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

constexpr size_t num_threads = 16;
constexpr size_t num_iterations = 2000;

void thread_func(const ze_command_queue_handle_t cq) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  const size_t memory_size = 64 * 1024 * 1024;

  void *output_buffer = lzt::allocate_shared_memory(memory_size);

  for (int i = 0; i < num_iterations; i++) {
    ze_fence_handle_t fence_ = lzt::create_fence(cq);

    const uint8_t pattern_base = std::rand() % 0xFF;
    const int pattern_size = 1;

    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    ze_command_list_handle_t cl = lzt::create_command_list();

    lzt::append_memory_fill(cl, output_buffer, &pattern_base, pattern_size,
                            memory_size, nullptr);

    lzt::close_command_list(cl);

    lzt::execute_command_lists(cq, 1, &cl, fence_);

    lzt::sync_fence(fence_, UINT64_MAX);

    lzt::query_fence(fence_);

    lzt::reset_fence(fence_);

    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    lzt::destroy_fence(fence_);

    lzt::destroy_command_list(cl);
  }
  lzt::synchronize(cq, UINT64_MAX);

  lzt::free_memory(output_buffer);

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeFenceSynchronizeTests : public ::testing::Test {};

TEST(
    zeFenceSynchronizeTests,
    GivenMultipleThreadsUsingSingleCommandQueueAndSynchronizingThenSuccessIsReturned) {

  ze_command_queue_handle_t cq = lzt::create_command_queue();

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::thread *> threads;

  for (int i = 0; i < num_threads; i++)
    threads.push_back(new std::thread(thread_func, cq));

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (int i = 0; i < num_threads; i++) {
    delete threads[i];
  }

  lzt::destroy_command_queue(cq);
}

} // namespace
