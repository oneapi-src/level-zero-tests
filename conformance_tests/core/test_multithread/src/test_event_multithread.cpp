/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"
#include <thread>
#include "utils/utils.hpp"
#include <array>
#include <mutex>

namespace lzt = level_zero_tests;

namespace {

constexpr size_t num_threads = 16;
constexpr size_t num_events = 16;
constexpr size_t num_iterations = 2000;
bool signal_device_event = true;
bool signal_host_event = true;
uint32_t thread_count = 0;
std::mutex event_mutex;

void ThreadEventCreate() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  // Each thread creates event pool,events and later destroy events

  ze_event_pool_desc_t eventPoolDesc = {};
  eventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  eventPoolDesc.count = num_events;

  for (uint32_t i = 0; i < num_iterations; i++) {
    ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
    std::array<ze_event_handle_t, num_events> events;
    std::array<ze_event_desc_t, num_events> eventDesc;
    for (uint32_t j = 0; j < num_events; j++) {
      eventDesc[j].stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
      eventDesc[j].pNext = nullptr;
      eventDesc[j].index = j;
      eventDesc[j].signal = ZE_EVENT_SCOPE_FLAG_HOST;
      eventDesc[j].wait = ZE_EVENT_SCOPE_FLAG_HOST;
      events[j] = lzt::create_event(event_pool, eventDesc[j]);
    }
    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (auto event : events) {
      lzt::signal_event_from_host(event);
    }
    for (auto event : events) {
      lzt::query_event(event);
    }
    for (auto event : events) {
      lzt::event_host_reset(event);
    }
    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (auto event : events) {
      lzt::destroy_event(event);
    }

    // As per the spec, index could be re-used by an event after destroy
    for (uint32_t j = 0; j < num_events; j++) {
      events[j] = lzt::create_event(event_pool, eventDesc[j]);
    }

    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (auto event : events) {
      lzt::destroy_event(event);
    }

    lzt::destroy_event_pool(event_pool);
  }
  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void ThreadEventSync() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  // Each thread creates event pool,events and uses them to sync
  // and later destroy events

  ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();

  ze_event_pool_desc_t eventPoolDesc = {};
  eventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  eventPoolDesc.count = num_events;

  ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
  std::array<ze_event_handle_t, num_events> events;
  std::array<ze_event_desc_t, num_events> eventDesc;
  std::array<ze_command_list_handle_t, num_events> cmd_list;
  for (uint32_t j = 0; j < num_events; j++) {
    eventDesc[j].stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    eventDesc[j].pNext = nullptr;
    eventDesc[j].index = j;
    eventDesc[j].signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc[j].wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events[j] = lzt::create_event(event_pool, eventDesc[j]);
    cmd_list[j] = lzt::create_command_list();
  }
  for (auto event : events) {
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
  }

  // As per the spec, index could be re-used after destroy
  for (uint32_t j = 0; j < num_events; j++) {
    lzt::destroy_event(events[j]);
    events[j] = lzt::create_event(event_pool, eventDesc[j]);
  }

  for (uint32_t i = 0; i < num_iterations; i++) {
    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }
    for (uint32_t j = 0; j < num_events; j++) {
      lzt::signal_event_from_host(events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
      lzt::event_host_reset(events[j]);

      lzt::synchronize(cmd_queue, UINT64_MAX);
      lzt::reset_command_list(cmd_list[j]);

      lzt::append_wait_on_events(cmd_list[j], 1, &events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::signal_event_from_host(events[j]);
      lzt::query_event(events[j]);

      lzt::synchronize(cmd_queue, UINT64_MAX);
      lzt::reset_command_list(cmd_list[j]);

      lzt::destroy_event(events[j]);
      events[j] = lzt::create_event(event_pool, eventDesc[j]);
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[j]));

      lzt::append_signal_event(cmd_list[j], events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
      lzt::event_host_reset(events[j]);

      lzt::synchronize(cmd_queue, UINT64_MAX);
      lzt::reset_command_list(cmd_list[j]);

      lzt::append_wait_on_events(cmd_list[j], 1, &events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::signal_event_from_host(events[j]);
      lzt::query_event(events[j]);

      lzt::synchronize(cmd_queue, UINT64_MAX);
      lzt::reset_command_list(cmd_list[j]);
      lzt::event_host_reset(events[j]);
    }
  }
  lzt::synchronize(cmd_queue, UINT64_MAX);

  for (auto event : events) {
    lzt::destroy_event(event);
  }
  lzt::destroy_event_pool(event_pool);
  for (auto cl : cmd_list) {
    lzt::destroy_command_list(cl);
  }
  lzt::destroy_command_queue(cmd_queue);
  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void ThreadMultipleEventsSync() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  // Each thread creates event pool,events and uses them to sync
  // and later destroy events

  ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();

  ze_event_pool_desc_t eventPoolDesc = {};
  eventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  eventPoolDesc.count = num_events;

  ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
  std::array<ze_event_handle_t, num_events> events;
  std::array<ze_event_desc_t, num_events> eventDesc;
  std::array<ze_command_list_handle_t, num_events> cmd_list;
  for (uint32_t j = 0; j < num_events; j++) {
    eventDesc[j].stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    eventDesc[j].pNext = nullptr;
    eventDesc[j].index = j;
    eventDesc[j].signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc[j].wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events[j] = lzt::create_event(event_pool, eventDesc[j]);
    cmd_list[j] = lzt::create_command_list();
  }

  for (uint32_t i = 0; i < num_iterations; i++) {
    for (auto event : events) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (uint32_t j = 0; j < num_events / 2; j++) {
      lzt::append_wait_on_events(cmd_list[j], 1, &events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::signal_event_from_host(events[j]);
      lzt::query_event(events[j]);
    }

    for (uint32_t j = num_events / 2; j < num_events; j++) {
      lzt::append_signal_event(cmd_list[j], events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
    }

    lzt::synchronize(cmd_queue, UINT64_MAX);

    // As per the spec, index could be re-used after destroy
    for (uint32_t j = 0; j < num_events; j++) {
      lzt::destroy_event(events[j]);
      events[j] = lzt::create_event(event_pool, eventDesc[j]);
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(events[j]));
    }

    for (uint32_t j = 0; j < num_events / 2; j++) {
      lzt::reset_command_list(cmd_list[j]);
      lzt::append_wait_on_events(cmd_list[j], 1, &events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::signal_event_from_host(events[j]);
      lzt::query_event(events[j]);
    }

    for (uint32_t j = num_events / 2; j < num_events; j++) {
      lzt::reset_command_list(cmd_list[j]);
      lzt::append_signal_event(cmd_list[j], events[j]);
      lzt::close_command_list(cmd_list[j]);
      lzt::execute_command_lists(cmd_queue, 1, &cmd_list[j], nullptr);
      lzt::event_host_synchronize(events[j], UINT64_MAX);
      lzt::query_event(events[j]);
    }

    lzt::synchronize(cmd_queue, UINT64_MAX);

    for (auto event : events) {
      lzt::event_host_reset(event);
    }
    for (auto cl : cmd_list) {
      lzt::reset_command_list(cl);
    }
  }

  lzt::synchronize(cmd_queue, UINT64_MAX);

  for (auto event : events) {
    lzt::destroy_event(event);
  }
  for (auto cl : cmd_list) {
    lzt::destroy_command_list(cl);
  }
  lzt::destroy_event_pool(event_pool);
  lzt::destroy_command_queue(cmd_queue);
  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void ThreadSharedEventSync(const ze_command_queue_handle_t cmd_queue,
                           std::array<ze_event_handle_t, num_events> event) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID ::" << thread_id;

  // Threads use shared event handles to sync amongst themselves
  for (uint32_t i = 0; i < num_iterations; i++) {

    ze_command_list_handle_t cmd_list = lzt::create_command_list();

    event_mutex.lock();
    if (signal_device_event) {
      signal_device_event = false;
      for (uint32_t index = 0; index < num_events - 1; index++) {
        lzt::append_signal_event(cmd_list, event[index]);
      }
    }
    event_mutex.unlock();
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);

    for (uint32_t index = 0; index < num_events - 1; index++) {
      lzt::event_host_synchronize(event[index], UINT64_MAX);
    }

    lzt::synchronize(cmd_queue, UINT64_MAX);
    lzt::reset_command_list(cmd_list);

    event_mutex.lock();
    thread_count++;
    if (thread_count == num_threads) {
      for (uint32_t index = 0; index < num_events - 1; index++) {
        lzt::query_event(event[index]);
        lzt::event_host_reset(event[index]);
      }
      thread_count = 0;
      lzt::signal_event_from_host(event[num_events - 1]);
    }
    event_mutex.unlock();

    lzt::event_host_synchronize(event[num_events - 1], UINT64_MAX);

    lzt::append_wait_on_events(cmd_list, num_events - 1, &event[1]);
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);

    event_mutex.lock();
    if (signal_host_event) {
      signal_host_event = false;
      for (uint32_t index = 1; index < num_events; index++) {
        lzt::signal_event_from_host(event[index]);
      }
    }
    event_mutex.unlock();

    lzt::synchronize(cmd_queue, UINT64_MAX);

    event_mutex.lock();
    thread_count++;
    if (thread_count == num_threads) {
      for (uint32_t index = 1; index < num_events; index++) {
        lzt::query_event(event[index]);
        lzt::event_host_reset(event[index]);
      }
      signal_device_event = true;
      signal_host_event = true;
      thread_count = 0;
      lzt::signal_event_from_host(event[0]);
    }
    event_mutex.unlock();

    lzt::event_host_synchronize(event[0], UINT64_MAX);

    lzt::synchronize(cmd_queue, UINT64_MAX);
    lzt::destroy_command_list(cmd_list);
  }
  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeEventSynchronizeTests : public ::testing::Test {};

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsCreateEventPoolAndEventsSimultaneouslyThenSuccessIsReturned) {

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = new std::thread(ThreadEventCreate);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (auto thread : threads) {
    delete thread;
  }
}

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsUsingSharedCommandQueueAndHavingTheirOwnCommandListAndEventsToSynchronizeThenSuccessIsReturned) {

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = new std::thread(ThreadEventSync);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i] = new std::thread(ThreadMultipleEventsSync);
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (auto thread : threads) {
    delete thread;
  }
}

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsUsingSharedCommandQueueAndSharedEventPoolAndHavingTheirOwnCommandListThenSynchronizeProperlyAndSuccessIsReturned) {

  ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();
  ze_event_pool_desc_t eventPoolDesc = {};
  eventPoolDesc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  eventPoolDesc.count = num_events;

  // As per the spec, the application must not call zeEventCreate() from
  // simultaneous threads having same pool handle.
  // Parent/ Root thread is expected to create all the events within the pool
  // and individual child threads then are expected to use the shared event
  // handles

  ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
  std::array<ze_event_handle_t, num_events> events;
  std::array<ze_event_desc_t, num_events> eventDesc;
  for (uint32_t j = 0; j < num_events; j++) {
    eventDesc[j].stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    eventDesc[j].pNext = nullptr;
    eventDesc[j].index = j;
    eventDesc[j].signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc[j].wait = ZE_EVENT_SCOPE_FLAG_HOST;
    events[j] = lzt::create_event(event_pool, eventDesc[j]);
  }

  for (auto event : events) {
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
  }

  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t j = 0; j < num_threads; j++) {
    threads[j] = new std::thread(ThreadSharedEventSync, cmd_queue, events);
  }

  for (uint32_t j = 0; j < num_threads; j++) {
    threads[j]->join();
  }
  for (auto event : events) {
    lzt::event_host_reset(event);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
  }

  for (auto thread : threads) {
    delete thread;
  }

  for (auto event : events) {
    lzt::event_host_reset(event);
  }

  for (auto event : events) {
    lzt::destroy_event(event);
  }

  lzt::destroy_event_pool(event_pool);
  lzt::destroy_command_queue(cmd_queue);
}

} // namespace
