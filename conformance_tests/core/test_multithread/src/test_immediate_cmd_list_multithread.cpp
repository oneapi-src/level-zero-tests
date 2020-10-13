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
constexpr size_t size = 4096;
constexpr size_t num_events = 16;

void thread_create_destroy_default() {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ze_command_list_handle_t cmd_list = lzt::create_immediate_command_list();
    lzt::close_command_list(cmd_list);
    lzt::destroy_command_list(cmd_list);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void thread_append_memory_copy() {
  std::thread::id thread_id = std::this_thread::get_id();

  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  // Each thread creates event pool,events and uses them to sync
  // and later destroy events

  ze_event_pool_desc_t eventPoolDesc = {};
  eventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  eventPoolDesc.count = num_events;

  for (uint32_t i = 0; i < thread_iters; i++) {
    ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
    std::array<ze_event_handle_t, num_events> events;
    std::array<ze_event_desc_t, num_events> eventDesc;
    std::array<ze_command_list_handle_t, num_events> cmdlist_immediate;
    std::array<uint8_t, size> host_memory1, host_memory2;
    for (uint32_t j = 0; j < num_events; j++) {
      eventDesc[j].stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      eventDesc[j].pNext = nullptr;
      eventDesc[j].index = j;
      eventDesc[j].signal = ZE_EVENT_SCOPE_FLAG_HOST;
      eventDesc[j].wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events[j] = lzt::create_event(event_pool, eventDesc[j]);
      cmdlist_immediate[j] = lzt::create_immediate_command_list();
    }

    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (uint32_t j = 0; j < num_events; j++) {
      void *device_memory = lzt::allocate_device_memory(size);
      lzt::write_data_pattern(host_memory1.data(), size, 1);
      // This should execute immediately
      lzt::append_memory_copy(cmdlist_immediate[j], device_memory,
                              host_memory1.data(), size, events[j]);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
      lzt::event_host_reset(events[j]);

      lzt::append_memory_copy(cmdlist_immediate[j], host_memory2.data(),
                              device_memory, size, events[j]);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
      lzt::validate_data_pattern(host_memory2.data(), size, 1);

      lzt::free_memory(device_memory);
    }
    for (auto event : events) {
      lzt::destroy_event(event);
    }
    for (auto cl : cmdlist_immediate) {
      lzt::destroy_command_list(cl);
    }
    lzt::destroy_event_pool(event_pool);
  }
}

class zeImmediateCommandListMultiThreadTest : public ::testing::Test {};

TEST(
    zeImmediateCommandListMultiTthreadTest,
    GivenMultipleThreadsWhenCreateCloseAndDestroyImmediateCommandListsThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_create_destroy_default);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

TEST(
    zeImmediateCommandListMultiThreadTest,
    GivenMultipleThreadsUseImmediateCommandListsForAppendMemoryCopyThenReturnSuccess) {
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::unique_ptr<std::thread>, num_threads> threads;

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = std::make_unique<std::thread>(thread_append_memory_copy);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }
}

} // namespace
