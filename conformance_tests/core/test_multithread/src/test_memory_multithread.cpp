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

constexpr uint32_t thread_iters = 200;
constexpr size_t num_threads = 8;
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

} // namespace
