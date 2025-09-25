/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

using lzt::to_u32;

class zeCommandListAppendBarrierTests : public ::testing::Test {
public:
  void run(bool isImmediate) {
    auto bundle = lzt::create_command_bundle(isImmediate);
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListAppendBarrier(bundle.list, nullptr, 0, nullptr));
    lzt::destroy_command_bundle(bundle);
  }
};

LZT_TEST_F(zeCommandListAppendBarrierTests,
           GivenEmptyCommandListWhenAppendingBarrierThenSuccessIsReturned) {
  run(false);
}

LZT_TEST_F(
    zeCommandListAppendBarrierTests,
    GivenEmptyImmediateCommandListWhenAppendingBarrierThenSuccessIsReturned) {
  run(true);
}

class zeEventPoolCommandListAppendBarrierTests : public ::testing::Test {};

void RunAppendingBarrierWithEvent(bool isImmediate) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
  ze_event_handle_t event = nullptr;
  ep.create_event(event);

  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendBarrier(bundle.list, event, 0, nullptr));
  ep.destroy_event(event);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithEventThenSuccessIsReturned) {
  RunAppendingBarrierWithEvent(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyImmediateCommandListWhenAppendingBarrierWithEventThenSuccessIsReturned) {
  RunAppendingBarrierWithEvent(true);
}

void RunAppendingBarrierWithEvents(bool isImmediate) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const size_t event_count = 2;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 2);
  std::vector<ze_event_handle_t> events(event_count, nullptr);
  ep.create_events(events, event_count);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);

  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(
      bundle.list, nullptr, to_u32(events.size()), events.data()));
  for (auto &ev : events) {
    lzt::signal_event_from_host(ev);
  }
  ep.destroy_events(events);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithEventsThenSuccessIsReturned) {
  RunAppendingBarrierWithEvents(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyImmediateCommandListWhenAppendingBarrierWithEventsThenSuccessIsReturned) {
  RunAppendingBarrierWithEvents(true);
}

void RunAppendingBarrierWithSignalEventAndWaitEvents(bool isImmediate) {
  lzt::zeEventPool ep;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event = nullptr;
  const size_t event_count = 2;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 3);
  std::vector<ze_event_handle_t> events(event_count, nullptr);

  ep.create_event(event);
  ep.create_events(events, event_count);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
  auto events_initial = events;
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(
      bundle.list, event, to_u32(events.size()), events.data()));
  for (auto &ev : events) {
    lzt::signal_event_from_host(ev);
  }
  for (size_t i = 0U; i < events.size(); i++) {
    ASSERT_EQ(events[i], events_initial[i]);
  }
  ep.destroy_events(events);
  ep.destroy_event(event);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyCommandListWhenAppendingBarrierWithSignalEventAndWaitEventsThenSuccessIsReturned) {
  RunAppendingBarrierWithSignalEventAndWaitEvents(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendBarrierTests,
    GivenEmptyImmediateCommandListWhenAppendingBarrierWithSignalEventAndWaitEventsThenSuccessIsReturned) {
  RunAppendingBarrierWithSignalEventAndWaitEvents(true);
}

void AppendMemoryRangesBarrierTest(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_list_handle_t command_list, ze_event_handle_t signaling_event,
    std::vector<ze_event_handle_t> &waiting_events) {
  const std::vector<size_t> range_sizes{4096, 8192};
  std::vector<const void *> ranges{
      lzt::allocate_device_memory((range_sizes[0] * 2), 1, 0, 0, device,
                                  context),
      lzt::allocate_device_memory((range_sizes[1] * 2), 1, 0, 0, device,
                                  context)};

  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryRangesBarrier(
      command_list, to_u32(ranges.size()), range_sizes.data(), ranges.data(),
      signaling_event, to_u32(waiting_events.size()), waiting_events.data()));

  for (auto &ev : waiting_events) {
    lzt::signal_event_from_host(ev);
  }

  for (auto &range : ranges)
    lzt::free_memory(context, range);
}

class zeCommandListAppendMemoryRangesBarrierTests : public ::testing::Test {
public:
  void run(bool isImmediate) {
    const ze_device_handle_t device =
        lzt::zeDevice::get_instance()->get_device();
    std::vector<ze_event_handle_t> waiting_events;
    ze_context_handle_t context = lzt::create_context();
    auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
    AppendMemoryRangesBarrierTest(context, device, bundle.list, nullptr,
                                  waiting_events);
    lzt::destroy_command_bundle(bundle);
    lzt::destroy_context(context);
  }
};

LZT_TEST_F(
    zeCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListWhenAppendingMemoryRangesBarrierThenSuccessIsReturned) {
  run(false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyImmediateCommandListWhenAppendingMemoryRangesBarrierThenSuccessIsReturned) {
  run(true);
}

class zeEventPoolCommandListAppendMemoryRangesBarrierTests
    : public ::testing::Test {};

void RunAppendingMemoryRangesBarrierSignalEvent(bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
  ze_event_handle_t signaling_event = nullptr;
  std::vector<ze_event_handle_t> waiting_events;

  ep.create_event(signaling_event);
  AppendMemoryRangesBarrierTest(context, device, bundle.list, signaling_event,
                                waiting_events);
  ep.destroy_event(signaling_event);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierSignalEvent(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyImmediateCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierSignalEvent(true);
}

void RunAppendingMemoryRangesBarrierWaitEvents(bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  const size_t event_count = 2;
  std::vector<ze_event_handle_t> waiting_events(event_count, nullptr);
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 2);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
  ep.create_events(waiting_events, event_count);
  auto wait_events_initial = waiting_events;
  AppendMemoryRangesBarrierTest(context, device, bundle.list, nullptr,
                                waiting_events);
  for (size_t i = 0U; i < waiting_events.size(); i++) {
    ASSERT_EQ(waiting_events[i], wait_events_initial[i]);
  }
  ep.destroy_events(waiting_events);
  lzt::destroy_command_list(bundle.list);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierWaitEventsThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierWaitEvents(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyImmediateCommandListEventPoolWhenAppendingMemoryRangesBarrierWaitEventsThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierWaitEvents(true);
}

void RunAppendingMemoryRangesBarrierSignalEventAndWaitEvents(bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  lzt::zeEventPool ep;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 3);
  auto bundle = lzt::create_command_bundle(context, device, 0, isImmediate);
  ze_event_handle_t signaling_event = nullptr;
  const size_t event_count = 2;

  std::vector<ze_event_handle_t> waiting_events(event_count, nullptr);
  ep.create_event(signaling_event);
  ep.create_events(waiting_events, event_count);
  AppendMemoryRangesBarrierTest(context, device, bundle.list, signaling_event,
                                waiting_events);
  ep.destroy_event(signaling_event);
  ep.destroy_events(waiting_events);
  lzt::destroy_command_bundle(bundle);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventAndWaitEventsThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierSignalEventAndWaitEvents(false);
}

LZT_TEST_F(
    zeEventPoolCommandListAppendMemoryRangesBarrierTests,
    GivenEmptyImmediateCommandListEventPoolWhenAppendingMemoryRangesBarrierSignalEventAndWaitEventsThenSuccessIsReturned) {
  RunAppendingMemoryRangesBarrierSignalEventAndWaitEvents(true);
}

class zeContextSystemBarrierTests : public ::testing::Test {};

LZT_TEST_F(zeContextSystemBarrierTests,
           GivenDeviceWhenAddingSystemBarrierThenSuccessIsReturned) {

  EXPECT_ZE_RESULT_SUCCESS(

      zeContextSystemBarrier(lzt::get_default_context(),
                             lzt::zeDevice::get_instance()->get_device()));
}

class zeBarrierEventSignalingTests : public lzt::zeEventPoolTests {};

void RunEventSignaledWhenAppendingBarrierThenHostDetectsEvent(
    lzt::zeEventPool ep, bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event_barrier_to_host = nullptr;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  auto bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, isImmediate);
  ep.create_event(event_barrier_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                  ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_memory_copy(bundle.list, dev_mem, inpa.data(), xfer_size,
                          nullptr);
  lzt::append_barrier(bundle.list, event_barrier_to_host, 0, nullptr);
  lzt::append_memory_copy(bundle.list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventHostSynchronize(event_barrier_to_host, UINT32_MAX - 1));
  EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event_barrier_to_host));

  lzt::destroy_command_bundle(bundle);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_event(event_barrier_to_host);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenCommandListAppendingBarrierThenHostDetectsEventSuccessfully) {
  RunEventSignaledWhenAppendingBarrierThenHostDetectsEvent(ep, false);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenImmediateCommandListAppendingBarrierThenHostDetectsEventSuccessfully) {
  RunEventSignaledWhenAppendingBarrierThenHostDetectsEvent(ep, true);
}

void RunAppendingBarrierWaitsForEventsWhenHostAndSendSignals(
    lzt::zeEventPool ep, bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t num_events = 6;
  ASSERT_GE(num_events, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 6);
  std::vector<ze_event_handle_t> events_to_barrier(num_events, nullptr);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  auto bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, isImmediate);
  ep.create_events(events_to_barrier, num_events, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_signal_event(bundle.list, events_to_barrier[0]);
  lzt::append_memory_copy(bundle.list, dev_mem, inpa.data(), xfer_size,
                          nullptr);
  lzt::append_signal_event(bundle.list, events_to_barrier[1]);
  lzt::append_barrier(bundle.list, nullptr, num_events,
                      events_to_barrier.data());
  lzt::append_memory_copy(bundle.list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(bundle.list);
  if (!isImmediate) {
    lzt::execute_command_lists(bundle.queue, 1, &bundle.list, nullptr);
  }

  for (uint32_t i = 2; i < num_events; i++) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(events_to_barrier[i]));
  }
  if (isImmediate) {
    lzt::synchronize_command_list_host(bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(bundle.queue, UINT64_MAX);
  }
  for (uint32_t i = 0; i < num_events; i++) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(events_to_barrier[i]));
  }

  lzt::destroy_command_bundle(bundle);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_events(events_to_barrier);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenCommandListAppendingBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  RunAppendingBarrierWaitsForEventsWhenHostAndSendSignals(ep, false);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenImmediateCommandListAppendingBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  RunAppendingBarrierWaitsForEventsWhenHostAndSendSignals(ep, true);
}

void RunEventSignaledWhenAppendingMemoryRangesBarrierThenHostDetectsEvent(
    lzt::zeEventPool ep, bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_event_handle_t event_barrier_to_host = nullptr;
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 1);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  const std::vector<size_t> range_sizes{inpa.size(), inpa.size()};
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  std::vector<const void *> ranges{dev_mem, host_mem};
  auto bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, isImmediate);
  ep.create_event(event_barrier_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                  ZE_EVENT_SCOPE_FLAG_HOST);
  lzt::append_memory_copy(bundle.list, dev_mem, inpa.data(), xfer_size,
                          nullptr);
  lzt::append_memory_ranges_barrier(bundle.list, to_u32(ranges.size()),
                                    range_sizes.data(), ranges.data(),
                                    event_barrier_to_host, 0, nullptr);
  lzt::append_memory_copy(bundle.list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventHostSynchronize(event_barrier_to_host, UINT32_MAX - 1));
  EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event_barrier_to_host));

  lzt::destroy_command_bundle(bundle);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_event(event_barrier_to_host);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenCommandListAppendingMemoryRangesBarrierThenHostDetectsEventSuccessfully) {
  RunEventSignaledWhenAppendingMemoryRangesBarrierThenHostDetectsEvent(ep,
                                                                       false);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenEventSignaledWhenImmediateCommandListAppendingMemoryRangesBarrierThenHostDetectsEventSuccessfully) {
  RunEventSignaledWhenAppendingMemoryRangesBarrierThenHostDetectsEvent(ep,
                                                                       true);
}

void RunAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndSendSignals(
    lzt::zeEventPool ep, bool isImmediate) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t num_events = 6;
  ASSERT_GE(num_events, 3);
  ze_context_handle_t context = lzt::create_context();
  ep.InitEventPool(context, 6);
  std::vector<ze_event_handle_t> events_to_barrier(num_events, nullptr);
  std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                           8, 9, 10, 11, 12, 13, 14, 15};
  size_t xfer_size = inpa.size() * sizeof(int);
  const std::vector<size_t> range_sizes{inpa.size(), inpa.size()};
  void *dev_mem =
      lzt::allocate_device_memory(xfer_size, 1, 0, 0, device, context);
  void *host_mem = lzt::allocate_host_memory(xfer_size, 1, context);
  std::vector<const void *> ranges{dev_mem, host_mem};
  auto bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, isImmediate);
  ep.create_events(events_to_barrier, num_events, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);

  lzt::append_signal_event(bundle.list, events_to_barrier[0]);
  lzt::append_memory_copy(bundle.list, dev_mem, inpa.data(), xfer_size,
                          nullptr);
  lzt::append_signal_event(bundle.list, events_to_barrier[1]);
  lzt::append_memory_ranges_barrier(bundle.list, to_u32(ranges.size()),
                                    range_sizes.data(), ranges.data(), nullptr,
                                    num_events, events_to_barrier.data());
  lzt::append_memory_copy(bundle.list, host_mem, dev_mem, xfer_size, nullptr);
  lzt::close_command_list(bundle.list);
  if (!isImmediate) {
    lzt::execute_command_lists(bundle.queue, 1, &bundle.list, nullptr);
  }

  for (uint32_t i = 2; i < num_events; i++) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(events_to_barrier[i]));
  }
  if (isImmediate) {
    lzt::synchronize_command_list_host(bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(bundle.queue, UINT64_MAX);
  }
  for (uint32_t i = 0; i < num_events; i++) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(events_to_barrier[i]));
  }

  lzt::destroy_command_bundle(bundle);
  lzt::free_memory(context, host_mem);
  lzt::free_memory(context, dev_mem);
  ep.destroy_events(events_to_barrier);
  lzt::destroy_context(context);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenCommandListAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  RunAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndSendSignals(ep,
                                                                      false);
}

LZT_TEST_F(
    zeBarrierEventSignalingTests,
    GivenImmediateCommandListAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully) {
  RunAppendingMemoryRangesBarrierWaitsForEventsWhenHostAndSendSignals(ep, true);
}

enum BarrierType {
  BT_GLOBAL_BARRIER,
  BT_MEMORY_RANGES_BARRIER,
  BT_EVENTS_BARRIER
};

class zeBarrierKernelTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<std::tuple<enum BarrierType, bool>> {
};

LZT_TEST_P(
    zeBarrierKernelTests,
    GivenKernelFunctionsAndMemoryCopyWhenBarrierInsertedThenExecutionCompletesSuccesfully) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_context_handle_t context = lzt::create_context();

  const bool isImmediate = std::get<1>(GetParam());
  auto bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, isImmediate);
  uint32_t num_int = 1000;
  void *dev_buff = lzt::allocate_device_memory((num_int * sizeof(int)), 1, 0, 0,
                                               device, context);
  void *host_buff =
      lzt::allocate_host_memory((num_int * sizeof(int)), 1, context);

  int *p_host = static_cast<int *>(host_buff);
  enum BarrierType barrier_type = std::get<0>(GetParam());

  // Initialize host buffer with postive and negative integer values
  const int addval_1 = -100;
  const int val_1 = 50000;
  int val = val_1;
  for (uint32_t i = 0; i < num_int; i++) {
    p_host[i] = val;
    val += addval_1;
  }

  // Setup 2 events if running EVENTS_BARRIER test.  Flags set for coherency
  // between device and host
  const size_t num_event = 2;
  std::vector<ze_event_handle_t> coherency_event(num_event, nullptr);
  ep.InitEventPool(context, 2);
  ep.create_events(coherency_event, num_event, ZE_EVENT_SCOPE_FLAG_HOST,
                   ZE_EVENT_SCOPE_FLAG_HOST);

  ze_event_handle_t signal_event_copy = nullptr;
  ze_event_handle_t signal_event_func1 = nullptr;
  uint32_t num_wait = 0;
  ze_event_handle_t *p_wait_event_func1 = nullptr;
  ze_event_handle_t *p_wait_event_func2 = nullptr;

  // Coherency test:
  // 1) Command list memcopy from Host to Device Buffer.
  // 2) First kernel function uses Device Buffer as input buffer and adds
  // constant 3) Second kernel function adds Device Buffer output of first to
  // Host buffer 4) Coherency via barriers or events needed betwen 1&2 and
  // between 2&3

  if (barrier_type == BT_EVENTS_BARRIER) {
    signal_event_copy = coherency_event[0];
    signal_event_func1 = coherency_event[1];
    num_wait = 1;
    p_wait_event_func1 = &coherency_event[0];
    p_wait_event_func2 = &coherency_event[1];
  }

  const int addval_2 = 50;
  ze_module_handle_t module =
      lzt::create_module(context, device, "barrier_add.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function_1 =
      lzt::create_function(module, "barrier_add_constant");
  lzt::set_group_size(function_1, 1, 1, 1);

  int *p_dev = static_cast<int *>(dev_buff);
  lzt::set_argument_value(function_1, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(function_1, 1, sizeof(addval_2), &addval_2);
  ze_group_count_t tg;
  tg.groupCountX = num_int;
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  lzt::append_memory_copy(bundle.list, dev_buff, host_buff,
                          num_int * sizeof(int), signal_event_copy);
  // Memory barrier to ensure memory coherency after copy to device memory
  if (barrier_type == BT_MEMORY_RANGES_BARRIER) {
    const std::vector<size_t> range_sizes{num_int * sizeof(int),
                                          num_int * sizeof(int)};
    std::vector<const void *> ranges{dev_buff, host_buff};
    lzt::append_memory_ranges_barrier(bundle.list, to_u32(ranges.size()),
                                      range_sizes.data(), ranges.data(),
                                      nullptr, 0, nullptr);
  } else if (barrier_type == BT_GLOBAL_BARRIER) {
    lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  }
  lzt::append_launch_function(bundle.list, function_1, &tg, signal_event_func1,
                              num_wait, p_wait_event_func1);
  // Execution barrier to ensure function_1 completes before function_2 starts
  if (barrier_type != BT_EVENTS_BARRIER) {
    lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  }
  ze_kernel_handle_t function_2 =
      lzt::create_function(module, "barrier_add_two_arrays");
  lzt::set_group_size(function_2, 1, 1, 1);

  lzt::set_argument_value(function_2, 0, sizeof(p_host), &p_host);
  lzt::set_argument_value(function_2, 1, sizeof(p_dev), &p_dev);
  lzt::append_launch_function(bundle.list, function_2, &tg, nullptr, num_wait,
                              p_wait_event_func2);

  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  val = (2 * val_1) + addval_2;
  for (uint32_t i = 0; i < num_int; i++) {
    EXPECT_EQ(p_host[i], val);
    val += (2 * addval_1);
  }

  lzt::destroy_function(function_2);
  lzt::destroy_function(function_1);
  lzt::destroy_module(module);
  lzt::destroy_command_bundle(bundle);
  lzt::free_memory(context, host_buff);
  lzt::free_memory(context, dev_buff);
  ep.destroy_events(coherency_event);
  lzt::destroy_context(context);
}

INSTANTIATE_TEST_SUITE_P(
    TestKernelWithBarrierAndMemoryRangesBarrier, zeBarrierKernelTests,
    ::testing::Combine(::testing::Values(BT_GLOBAL_BARRIER,
                                         BT_MEMORY_RANGES_BARRIER,
                                         BT_EVENTS_BARRIER),
                       ::testing::Bool()));

} // namespace
