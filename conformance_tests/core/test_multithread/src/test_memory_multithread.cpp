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
constexpr size_t alloc_size = 8192;

void host_memory_create_destroy_thread() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *ptr = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ptr = lzt::allocate_host_memory(alloc_size);
    lzt::free_memory(ptr);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void shared_memory_create_destroy_thread() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *ptr = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ptr = lzt::allocate_shared_memory(alloc_size);
    lzt::free_memory(ptr);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void device_memory_create_destroy_thread() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *ptr = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ptr = lzt::allocate_device_memory(alloc_size);
    lzt::free_memory(ptr);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void all_memory_create_destroy_thread(const ze_command_queue_handle_t cq) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *device_ptr = nullptr;
  void *host_ptr = nullptr;
  void *shared_ptr = nullptr;
  for (uint32_t i = 0; i < thread_iters; i++) {
    device_ptr = lzt::allocate_device_memory(alloc_size);
    host_ptr = lzt::allocate_host_memory(alloc_size);
    shared_ptr = lzt::allocate_shared_memory(alloc_size);
    lzt::free_memory(device_ptr);
    lzt::free_memory(host_ptr);
    lzt::free_memory(shared_ptr);
  }
}

void all_memory_create_destroy_submissions_thread(
    const ze_command_queue_handle_t cq) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *device_ptr = nullptr;
  void *host_ptr = nullptr;
  void *shared_ptr = nullptr;

  for (uint32_t i = 0; i < thread_iters; i++) {
    const uint8_t pattern_base = std::rand() % 0xFF;
    const int pattern_size = 1;

    ze_fence_handle_t fence_ = lzt::create_fence(cq);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    device_ptr = lzt::allocate_device_memory(alloc_size);
    host_ptr = lzt::allocate_host_memory(alloc_size);
    shared_ptr = lzt::allocate_shared_memory(alloc_size);

    ze_command_list_handle_t cl = lzt::create_command_list();
    lzt::append_memory_fill(cl, shared_ptr, &pattern_base, pattern_size,
                            alloc_size, nullptr);
    lzt::append_memory_fill(cl, host_ptr, &pattern_base, pattern_size,
                            alloc_size, nullptr);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, fence_);
    lzt::sync_fence(fence_, UINT32_MAX);
    lzt::query_fence(fence_);
    lzt::reset_fence(fence_);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    lzt::append_memory_copy(cl, device_ptr, shared_ptr, alloc_size, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, device_ptr, host_ptr, alloc_size, nullptr);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, fence_);
    lzt::sync_fence(fence_, UINT32_MAX);
    lzt::query_fence(fence_);
    lzt::reset_fence(fence_);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    lzt::destroy_fence(fence_);
    lzt::destroy_command_list(cl);
    lzt::free_memory(device_ptr);
    lzt::free_memory(host_ptr);
    lzt::free_memory(shared_ptr);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeMemoryCreateDestroyMultithreadTest : public ::testing::Test {};

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenCreatingHostMemoryThenMemoryCreatedAndDestroyedSuccessfully) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] =
        std::make_unique<std::thread>(host_memory_create_destroy_thread);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenCreatingSharedMemoryThenMemoryCreatedAndDestroyedSuccessfully) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] =
        std::make_unique<std::thread>(shared_memory_create_destroy_thread);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenCreatingDeviceMemoryThenMemoryCreatedAndDestroyedSuccessfully) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] =
        std::make_unique<std::thread>(device_memory_create_destroy_thread);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenUsingAllMemoryTypesThenMemoryCreatedAndDestroyedSuccessfully) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  ze_command_queue_handle_t cq = lzt::create_command_queue();

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] =
        std::make_unique<std::thread>(all_memory_create_destroy_thread, cq);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_command_queue(cq);
}

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenUsingAllMemoryTypesAlongWithSubmissionsThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  ze_command_queue_handle_t cq = lzt::create_command_queue();

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(
        all_memory_create_destroy_submissions_thread, cq);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_command_queue(cq);
}

} // namespace
