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

namespace lzt = level_zero_tests;

namespace {

constexpr size_t num_threads = 8;
constexpr size_t num_events = 2;
constexpr size_t num_iterations = 100;
bool signal_device_event = true;
bool signal_host_event = true;

void ThreadEventCreate() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_INFO << "child thread spawned with ID ::" << thread_id;

  // Each thread creates event pool,events and later destroy events

  ze_event_pool_desc_t eventPoolDesc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                        ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                                        num_events};
  for (uint32_t i = 0; i < num_iterations; i++) {
    ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
    std::array<ze_event_handle_t, num_events> event;
    std::array<ze_event_desc_t, num_events> eventDesc;
    for (uint32_t j = 0; j < num_events; j++) {
      eventDesc[j] = {ZE_EVENT_DESC_VERSION_CURRENT, j,
                      ZE_EVENT_SCOPE_FLAG_NONE, ZE_EVENT_SCOPE_FLAG_HOST};
      event[j] = lzt::create_event(event_pool, eventDesc[j]);
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[j]));
    }

    for (uint32_t k = 0; k < num_events; k++) {
      lzt::signal_event_from_host(event[k]);
    }
    for (uint32_t k = 0; k < num_events; k++) {
      lzt::query_event(event[k]);
    }
    for (uint32_t k = 0; k < num_events; k++) {
      lzt::event_host_reset(event[k]);
    }
    for (uint32_t k = 0; k < num_events; k++) {
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[k]));
    }

    for (uint32_t k = 0; k < num_events; k++) {
      lzt::destroy_event(event[k]);
    }
    lzt::destroy_event_pool(event_pool);
  }
  LOG_INFO << "child thread done with ID ::" << std::this_thread::get_id();
}

void ThreadEventSync(const ze_command_queue_handle_t cmd_queue) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_INFO << "child thread spawned with ID ::" << thread_id;

  // Each thread creates event pool,events and uses them to sync
  // and later destroy events

  ze_event_pool_desc_t eventPoolDesc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                        ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                                        num_events};

  for (uint32_t i = 0; i < num_iterations; i++) {
    ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);
    std::array<ze_event_handle_t, num_events> event;
    std::array<ze_event_desc_t, num_events> eventDesc;
    std::array<ze_command_list_handle_t, num_events> cmd_list;
    for (uint32_t j = 0; j < num_events; j++) {
      eventDesc[j] = {ZE_EVENT_DESC_VERSION_CURRENT, j,
                      ZE_EVENT_SCOPE_FLAG_NONE, ZE_EVENT_SCOPE_FLAG_HOST};
      event[j] = lzt::create_event(event_pool, eventDesc[j]);
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[j]));
      cmd_list[j] = lzt::create_command_list();
    }

    lzt::append_signal_event(cmd_list[0], event[0]);
    lzt::close_command_list(cmd_list[0]);
    lzt::execute_command_lists(cmd_queue, 1, &cmd_list[0], nullptr);
    lzt::event_host_synchronize(event[0], UINT32_MAX);
    lzt::query_event(event[0]);

    lzt::append_wait_on_events(cmd_list[1], 1, &event[1]);
    lzt::close_command_list(cmd_list[1]);
    lzt::execute_command_lists(cmd_queue, 1, &cmd_list[1], nullptr);
    lzt::signal_event_from_host(event[1]);
    lzt::query_event(event[1]);

    for (uint32_t k = 0; k < num_events; k++) {
      lzt::destroy_event(event[k]);
      lzt::destroy_command_list(cmd_list[k]);
    }
    lzt::destroy_event_pool(event_pool);
  }
  LOG_INFO << "child thread done with ID ::" << std::this_thread::get_id();
}

void ThreadSharedEventSync(const ze_command_queue_handle_t cmd_queue,
                           ze_event_pool_handle_t event_pool) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_INFO << "child thread spawned with ID ::" << thread_id;

  // Each thread uses shared event pool, creates events and uses them to sync
  // and later destroy events

  std::array<ze_event_handle_t, num_events> event;
  std::array<ze_event_desc_t, num_events> eventDesc;
  for (uint32_t j = 0; j < num_events; j++) {
    eventDesc[j] = {ZE_EVENT_DESC_VERSION_CURRENT, j, ZE_EVENT_SCOPE_FLAG_NONE,
                    ZE_EVENT_SCOPE_FLAG_HOST};
    event[j] = lzt::create_event(event_pool, eventDesc[j]);
  }
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[0]));
  ze_command_list_handle_t cmd_list = lzt::create_command_list();

  if (signal_device_event) {
    lzt::append_signal_event(cmd_list, event[0]);
    signal_device_event = false;
  }
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);
  lzt::event_host_synchronize(event[0], UINT32_MAX);
  lzt::query_event(event[0]);

  lzt::reset_command_list(cmd_list);
  lzt::append_wait_on_events(cmd_list, 1, &event[1]);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);
  if (signal_host_event) {
    lzt::signal_event_from_host(event[1]);
    signal_host_event = false;
  }
  lzt::event_host_synchronize(event[1], UINT32_MAX);
  lzt::query_event(event[1]);

  lzt::reset_command_list(cmd_list);
  lzt::append_reset_event(cmd_list, event[0]);
  lzt::close_command_list(cmd_list);
  lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);
  lzt::event_host_reset(event[1]);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[0]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[1]));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event[2]));

  lzt::destroy_command_list(cmd_list);
  LOG_INFO << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeFenceSynchronizeTests : public ::testing::Test {};

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsCreateEventPoolAndEventsSimultaneouslyThenSuccessIsReturned) {

  LOG_INFO << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t i = 0; i < num_threads; i++)
    threads[i] = new std::thread(ThreadEventCreate);

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    delete threads[i];
  }
}

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsUsingSharedCommandQueueAndHavingTheirOwnCommandListAndEventsToSynchronizeThenSuccessIsReturned) {

  ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();

  LOG_INFO << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t i = 0; i < num_threads; i++)
    threads[i] = new std::thread(ThreadEventSync, cmd_queue);

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    delete threads[i];
  }

  lzt::destroy_command_queue(cmd_queue);
}

TEST(
    zeEventSynchronizeTests,
    GivenMultipleThreadsUsingSharedCommandQueueAndSharedEventPoolAndHavingTheirOwnCommandListThenSynchronizeProperlyAndSuccessIsReturned) {

  ze_command_queue_handle_t cmd_queue = lzt::create_command_queue();
  ze_event_pool_desc_t eventPoolDesc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                        ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
                                        num_events};
  ze_event_pool_handle_t event_pool = lzt::create_event_pool(eventPoolDesc);

  LOG_INFO << "Total number of threads spawned ::" << num_threads;

  std::array<std::thread *, num_threads> threads;
  for (uint32_t i = 0; i < num_threads; i++)
    threads[i] = new std::thread(ThreadSharedEventSync, cmd_queue, event_pool);

  for (uint32_t i = 0; i < num_threads; i++) {
    threads[i]->join();
  }

  for (uint32_t i = 0; i < num_threads; i++) {
    delete threads[i];
  }

  lzt::destroy_event_pool(event_pool);
  lzt::destroy_command_queue(cmd_queue);
}

} // namespace
