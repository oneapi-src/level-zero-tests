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
constexpr size_t alloc_size = 4096;

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
    lzt::sync_fence(fence_, UINT64_MAX);
    lzt::query_fence(fence_);
    lzt::reset_fence(fence_);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    lzt::reset_command_list(cl);
    lzt::append_memory_copy(cl, device_ptr, shared_ptr, alloc_size, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, device_ptr, host_ptr, alloc_size, nullptr);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, fence_);
    lzt::sync_fence(fence_, UINT64_MAX);
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

void perform_memory_manipulation() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  void *device_ptr = nullptr;
  void *host_ptr = nullptr;
  void *shared_ptr = nullptr;

  const uint8_t pattern_base = std::rand() % 0xFF;
  const int pattern_size = 1;

  lzt::zeEventPool ep;
  ze_command_queue_handle_t cq = lzt::create_command_queue();
  ze_fence_handle_t fence_ = lzt::create_fence(cq);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

  device_ptr = lzt::allocate_device_memory(alloc_size);
  host_ptr = lzt::allocate_host_memory(alloc_size);
  shared_ptr = lzt::allocate_shared_memory(alloc_size);

  ze_event_handle_t event_barrier_to_host = nullptr;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_device_handle_t device_test = device;

  ze_memory_allocation_properties_t memory_properties = {};
  memory_properties.pNext = nullptr;
  memory_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  lzt::get_mem_alloc_properties(context, device_ptr, &memory_properties,
                                &device_test);
  EXPECT_EQ(ZE_MEMORY_TYPE_DEVICE, memory_properties.type);
  EXPECT_EQ(device, device_test);
  lzt::get_mem_alloc_properties(context, host_ptr, &memory_properties, nullptr);
  EXPECT_EQ(ZE_MEMORY_TYPE_HOST, memory_properties.type);
  lzt::get_mem_alloc_properties(context, shared_ptr, &memory_properties,
                                nullptr);
  EXPECT_EQ(ZE_MEMORY_TYPE_SHARED, memory_properties.type);

  void *pBase = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, device_ptr, &pBase, NULL));
  EXPECT_EQ(pBase, device_ptr);
  size_t addr_range_size = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, device_ptr, NULL, &addr_range_size));
  // Get device mem size rounds size up to nearest page size
  EXPECT_GE(addr_range_size, alloc_size);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, host_ptr, &pBase, NULL));
  EXPECT_EQ(pBase, host_ptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, host_ptr, NULL, &addr_range_size));
  EXPECT_GE(addr_range_size, alloc_size);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, shared_ptr, &pBase, NULL));
  EXPECT_EQ(pBase, shared_ptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(context, shared_ptr, NULL, &addr_range_size));
  EXPECT_GE(addr_range_size, alloc_size);

  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  const std::vector<size_t> range_sizes{inpa.size(), inpa.size()};
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  std::vector<const void *> ranges{dev_mem, host_mem};
  ze_command_list_handle_t cmd_list =
      lzt::create_command_list(context, device, 0);
  ep.create_event(event_barrier_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                  ZE_EVENT_SCOPE_FLAG_HOST);

  ze_command_list_handle_t cl = lzt::create_command_list();
  for (uint32_t i = 0; i < thread_iters; i++) {
    lzt::append_memory_fill(cl, shared_ptr, &pattern_base, pattern_size,
                            alloc_size, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_fill(cl, host_ptr, &pattern_base, pattern_size,
                            alloc_size, nullptr);
    lzt::append_memory_copy(cl, device_ptr, shared_ptr, alloc_size, nullptr);
    lzt::append_barrier(cl);
    lzt::append_memory_copy(cl, device_ptr, host_ptr, alloc_size, nullptr);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, fence_);
    lzt::sync_fence(fence_, UINT64_MAX);
    lzt::query_fence(fence_);
    lzt::reset_fence(fence_);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeFenceQueryStatus(fence_));

    lzt::append_memory_copy(cmd_list, dev_mem, inpa.data(), xfer_size, nullptr);
    lzt::append_memory_ranges_barrier(cmd_list, ranges.size(),
                                      range_sizes.data(), ranges.data(),
                                      event_barrier_to_host, 0, nullptr);
    lzt::append_memory_copy(cmd_list, host_mem, dev_mem, xfer_size, nullptr);
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cq, 1, &cmd_list, nullptr);
    lzt::synchronize(cq, UINT32_MAX);
    lzt::event_host_synchronize(event_barrier_to_host, UINT32_MAX - 1);
    lzt::query_event(event_barrier_to_host);

    lzt::reset_command_list(cl);
    lzt::reset_command_list(cmd_list);
    lzt::event_host_reset(event_barrier_to_host);
  }

  lzt::destroy_fence(fence_);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_event(event_barrier_to_host);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_list(cmd_list);
  lzt::free_memory(device_ptr);
  lzt::free_memory(host_ptr);
  lzt::free_memory(shared_ptr);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);
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

TEST(
    zeMemoryCreateDestroyMultithreadTest,
    GivenMultipleThreadsWhenPerformingMemoryManipulationsAlongWithSubmissionsThenSuccessIsReturned) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(perform_memory_manipulation);
  }

  for (int i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

} // namespace
