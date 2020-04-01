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

#include <level_zero/ze_api.h>
#include <thread>

namespace {

class EventProfilingTests : public ::testing::Test {
protected:
  EventProfilingTests() {

    const int ep_size = 10;
    ze_event_pool_desc_t ep_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                    ZE_EVENT_POOL_FLAG_TIMESTAMP, ep_size};
    ep = lzt::create_event_pool(ep_desc);
    ze_event_desc_t event_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 0,
                                  ZE_EVENT_SCOPE_FLAG_NONE,
                                  ZE_EVENT_SCOPE_FLAG_NONE};
    event = lzt::create_event(ep, event_desc);
    EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));

    cmdlist = lzt::create_command_list();
    cmdqueue = lzt::create_command_queue();
  }

  ~EventProfilingTests() {
    lzt::destroy_event(event);
    lzt::destroy_event_pool(ep);
    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);
  }
  ze_event_pool_handle_t ep;
  ze_event_handle_t event;
  ze_command_list_handle_t cmdlist;
  ze_command_queue_handle_t cmdqueue;
};

TEST_F(
    EventProfilingTests,
    GivenProfilingEventWhenCommandCompletesThenTimestampsAreRelationallyCorrect) {
  auto buffer = lzt::allocate_host_memory(100000);
  const uint8_t value = 0x55;

  lzt::append_memory_set(cmdlist, buffer, &value, 100000, event);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT32_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));

  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_START),
            0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_END), 0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_START),
            0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_END), 0);

  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_END),
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_START));
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_END),
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_START));

  lzt::free_memory(buffer);
}

TEST_F(
    EventProfilingTests,
    GivenSetProfilingEventWhenResettingEventThenEventReadsNotSignaledAndTimeStampsReadZero) {

  auto buffer = lzt::allocate_host_memory(100000);
  const uint8_t value = 0x55;

  lzt::append_memory_set(cmdlist, buffer, &value, 100000, event);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT32_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostReset(event));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));

  EXPECT_EQ(0,
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_START));
  EXPECT_EQ(0, lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_END));
  EXPECT_EQ(0,
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_START));
  EXPECT_EQ(0, lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_END));

  lzt::free_memory(buffer);
}

TEST_F(EventProfilingTests,
       GivenRegularAndTimestampEventPoolsWhenProfilingThenTimestampsStillWork) {
  const int mem_size = 1000;
  auto buffer = lzt::allocate_host_memory(mem_size);
  const uint8_t value1 = 0x55;
  const uint8_t value2 = 0x22;

  ze_event_pool_desc_t ep_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                  ZE_EVENT_POOL_FLAG_DEFAULT, 10};
  auto ep_no_timestamps = lzt::create_event_pool(ep_desc);
  ze_event_desc_t event_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 1,
                                ZE_EVENT_SCOPE_FLAG_HOST,
                                ZE_EVENT_SCOPE_FLAG_HOST};
  auto regular_event = lzt::create_event(ep, event_desc);

  lzt::append_memory_set(cmdlist, buffer, &value1, mem_size, regular_event);
  lzt::append_wait_on_events(cmdlist, 1, &regular_event);
  lzt::append_memory_set(cmdlist, buffer, &value2, mem_size, event);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT32_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));

  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_START),
            0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_END), 0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_START),
            0);
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_END), 0);

  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_END),
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_GLOBAL_START));
  EXPECT_GT(lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_END),
            lzt::get_event_timestamp(event, ZE_EVENT_TIMESTAMP_CONTEXT_START));

  for (int i = 0; i++; i < mem_size) {
    uint8_t byte = ((uint8_t *)buffer)[i];
    ASSERT_EQ(byte, value2);
  }

  lzt::free_memory(buffer);
  lzt::destroy_event(regular_event);
  lzt::destroy_event_pool(ep_no_timestamps);
}

class EventProfilingCacheCoherencyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_event_pool_flag_t> {};

TEST_P(EventProfilingCacheCoherencyTests,
       GivenEventWhenUsingEventToSyncThenCacheIsCoherent) {

  const uint32_t size = 10000;
  const uint8_t value = 0x55;

  ze_event_pool_desc_t ep_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                  GetParam(), 10};
  auto ep = lzt::create_event_pool(ep_desc);

  auto cmdlist = lzt::create_command_list();
  auto cmdqueue = lzt::create_command_queue();

  auto buffer1 = lzt::allocate_device_memory(size);
  auto buffer2 = lzt::allocate_device_memory(size);
  auto buffer3 = lzt::allocate_host_memory(size);
  auto buffer4 = lzt::allocate_device_memory(size);
  auto buffer5 = lzt::allocate_host_memory(size);
  memset(buffer3, 0, size);
  memset(buffer5, 0, size);

  ze_event_desc_t event_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 1,
                                ZE_EVENT_SCOPE_FLAG_HOST,
                                ZE_EVENT_SCOPE_FLAG_HOST};
  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 2;
  auto event2 = lzt::create_event(ep, event_desc);
  event_desc.index = 3;
  auto event3 = lzt::create_event(ep, event_desc);
  event_desc.index = 4;
  auto event4 = lzt::create_event(ep, event_desc);
  event_desc.index = 5;
  auto event5 = lzt::create_event(ep, event_desc);

  lzt::append_memory_set(cmdlist, buffer1, &value, size, event1);
  lzt::append_wait_on_events(cmdlist, 1, &event1);
  lzt::append_memory_copy(cmdlist, buffer2, buffer1, size,
                          event2); // Note: Need to add wait on events
  lzt::append_wait_on_events(cmdlist, 1, &event2);
  lzt::append_memory_copy(cmdlist, buffer3, buffer2, size, event3);
  lzt::append_wait_on_events(cmdlist, 1, &event3);
  lzt::append_memory_copy(cmdlist, buffer4, buffer3, size, event4);
  lzt::append_wait_on_events(cmdlist, 1, &event4);
  lzt::append_memory_copy(cmdlist, buffer5, buffer4, size, event5);

  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event5, UINT32_MAX));
  uint8_t *output = new uint8_t[size];
  memcpy(output, buffer5, size);

  for (int i = 0; i < size; i++) {
    ASSERT_EQ(output[i], value);
  }

  delete[] output;

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
  lzt::destroy_event(event3);
  lzt::destroy_event(event4);
  lzt::destroy_event(event5);
  lzt::destroy_event_pool(ep);
  lzt::free_memory(buffer1);
  lzt::free_memory(buffer2);
  lzt::free_memory(buffer3);
  lzt::free_memory(buffer4);
  lzt::free_memory(buffer5);
}

INSTANTIATE_TEST_CASE_P(CacheCoherencyTimeStampEventVsRegularEvent,
                        EventProfilingCacheCoherencyTests,
                        ::testing::Values(ZE_EVENT_POOL_FLAG_TIMESTAMP,
                                          ZE_EVENT_POOL_FLAG_DEFAULT));

} // namespace
