/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

#include <chrono>
#include <thread>
#include <bitset>

namespace {

class zeCommandListEventTests
    : public ::testing::TestWithParam<std::tuple<bool, bool>> {
public:
  zeCommandListEventTests() {
    ep.create_event(hEvent, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  }

  void TearDown() override { ep.destroy_event(hEvent); }

  ze_event_handle_t hEvent;
  size_t size = 16;
  lzt::zeEventPool ep;
};

class zeCommandListEventTestsExtended
    : public ::testing::TestWithParam<std::tuple<bool, bool, bool>> {
public:
  zeCommandListEventTestsExtended() {
    ep.create_event(hEvent, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  }

  void TearDown() override { ep.destroy_event(hEvent); }

  ze_event_handle_t hEvent;
  size_t size = 16;
  lzt::zeEventPool ep;
};

void RunGivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_copy(cmd_bundle.list, dst_buffer, src_buffer, test.size,
                          test.hEvent);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(test.hEvent, UINT64_MAX));

  // Verify Memory Copy completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto ref_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(ref_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);
  const uint8_t one = 1;

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_set(cmd_bundle.list, dst_buffer, &one, test.size,
                         test.hEvent);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(test.hEvent, UINT64_MAX));

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(ref_buffer, dst_buffer, test.size));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  lzt::free_memory_with_allocator_selector(ref_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySetThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  uint32_t width = 16;
  uint32_t height = 16;
  test.size = height * width;
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  ze_copy_region_t sr = {0U, 0U, 0U, width, height, 0U};
  ze_copy_region_t dr = {0U, 0U, 0U, width, height, 0U};
  lzt::append_memory_copy_region(cmd_bundle.list, dst_buffer, &dr, width, 0,
                                 src_buffer, &sr, width, 0, test.hEvent);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Host Reads Event as set
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(test.hEvent, UINT64_MAX));

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemoryCopiesWithDependenciesWhenExecutingCommandListTest(
    zeCommandListEventTestsExtended &test, bool is_immediate,
    bool is_shared_system_src, bool is_shared_system_temp,
    bool is_shared_system_dst) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto temp_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_temp);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(temp_buffer, 0xFF, test.size);
  memset(dst_buffer, 0x0, test.size);
  ze_event_handle_t hEvent1;
  test.ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_copy(cmd_bundle.list, temp_buffer, src_buffer, test.size,
                          test.hEvent, 0, nullptr);
  lzt::append_memory_copy(cmd_bundle.list, dst_buffer, temp_buffer, test.size,
                          nullptr, 1, &hEvent1);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Copy Waits for Signal
  lzt::event_host_synchronize(test.hEvent, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(hEvent1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(src_buffer, dst_buffer, test.size));
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(hEvent1));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Copy completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));

  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(temp_buffer, is_shared_system_temp);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_event(hEvent1);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTestsExtended,
    GivenMemoryCopiesWithDependenciesWhenExecutingCommandListThenCommandsCompletesSuccessfully) {
  RunGivenMemoryCopiesWithDependenciesWhenExecutingCommandListTest(
      *this, false, false, false, false);
}

TEST_F(
    zeCommandListEventTestsExtended,
    GivenMemoryCopiesWithDependenciesWhenExecutingImmediateCommandListThenCommandsCompletesSuccessfully) {
  RunGivenMemoryCopiesWithDependenciesWhenExecutingCommandListTest(
      *this, true, false, false, false);
}

TEST_P(
    zeCommandListEventTestsExtended,
    GivenMemoryCopiesWithDependenciesWhenExecutingCommandListThenCommandsCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopiesWithDependenciesWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()),
      std::get<2>(GetParam()));
}

TEST_P(
    zeCommandListEventTestsExtended,
    GivenMemoryCopiesWithDependenciesWhenExecutingImmediateCommandListThenCommandsCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopiesWithDependenciesWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()),
      std::get<2>(GetParam()));
}

void RunGivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  // This test is similar to the previous except that there is an
  // added delay to specifically test the wait functionality

  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_copy(cmd_bundle.list, dst_buffer, src_buffer, test.size,
                          nullptr, 1, &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // This sleep simulates work (e.g. file i/o) on the host that would cause
  // with a high probability the device to have to wait
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(src_buffer, dst_buffer, test.size));
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(test.hEvent));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Copy completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));
  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListTest(*this, false,
                                                                 false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListTest(*this, true,
                                                                 false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {

  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto ref_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(ref_buffer, 0x1, test.size);
  const uint8_t zero = 0;
  const uint8_t one = 1;

  ze_event_handle_t hEvent1;
  test.ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_fill(cmd_bundle.list, dst_buffer, &zero, sizeof(zero),
                          test.size, test.hEvent, 0, nullptr);
  lzt::append_memory_fill(cmd_bundle.list, dst_buffer, &one, sizeof(one),
                          test.size, nullptr, 1, &hEvent1);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  lzt::event_host_synchronize(test.hEvent, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(hEvent1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(ref_buffer, dst_buffer, test.size));
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(hEvent1));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Fill completed
  EXPECT_EQ(0, memcmp(ref_buffer, dst_buffer, test.size));

  lzt::free_memory_with_allocator_selector(ref_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_event(hEvent1);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListThenCommandCompletesSuccessfully) {
  RunGivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListTest(
      *this, false, false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryFillsThatSignalAndWaitWhenExecutingImmediateCommandListThenCommandCompletesSuccessfully) {
  RunGivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListTest(
      *this, true, false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListThenCommandCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryFillsThatSignalAndWaitWhenExecutingImmediateCommandListThenCommandCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemoryFillThatWaitsOnEventWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  // This test is similar to the previous except that there is an
  // added delay to specifically test the wait functionality

  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto ref_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(ref_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);
  const uint8_t one = 1;

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify GPU reads event
  lzt::append_memory_fill(cmd_bundle.list, dst_buffer, &one, sizeof(one),
                          test.size, nullptr, 1, &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Device waits for Signal
  // This sleep simulates work (e.g. file i/o) on the host that would cause
  // with a high probability the device to have to wait
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(ref_buffer, dst_buffer, test.size));
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(test.hEvent));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Fill completed
  EXPECT_EQ(0, memcmp(ref_buffer, dst_buffer, test.size));

  lzt::free_memory_with_allocator_selector(ref_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryFillThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryFillThatWaitsOnEventWhenExecutingCommandListTest(*this, false,
                                                                 false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryFillThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryFillThatWaitsOnEventWhenExecutingCommandListTest(*this, true,
                                                                 false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryFillThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryFillThatWaitsOnEventWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryFillThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryFillThatWaitsOnEventWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

void RunGivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListTest(
    zeCommandListEventTestsExtended &test, bool is_immediate,
    bool is_shared_system_src, bool is_shared_system_temp,
    bool is_shared_system_dst) {
  uint32_t width = 16;
  uint32_t height = 16;
  test.size = height * width;

  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto temp_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_temp);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(temp_buffer, 0xFF, test.size);
  memset(dst_buffer, 0x0, test.size);
  ze_event_handle_t hEvent1;
  test.ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify Device reads event
  ze_copy_region_t sr = {0U, 0U, 0U, width, height, 0U};
  ze_copy_region_t dr = {0U, 0U, 0U, width, height, 0U};
  lzt::append_memory_copy_region(cmd_bundle.list, temp_buffer, &dr, width, 0,
                                 src_buffer, &sr, width, 0, test.hEvent, 0,
                                 nullptr);
  lzt::append_memory_copy_region(cmd_bundle.list, dst_buffer, &dr, width, 0,
                                 temp_buffer, &sr, width, 0, nullptr, 1,
                                 &hEvent1);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Copy Waits for Signal
  lzt::event_host_synchronize(test.hEvent, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(hEvent1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(src_buffer, dst_buffer, test.size));
  }

  // Signal Event On Host
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(hEvent1));
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));
  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(temp_buffer, is_shared_system_temp);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_event(hEvent1);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTestsExtended,
    GivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListThenCommandCompletesSuccessfully) {
  RunGivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListTest(
      *this, false, false, false, false);
}

TEST_F(
    zeCommandListEventTestsExtended,
    GivenMemoryCopyRegionWithDependenciesWhenExecutingImmediateCommandListThenCommandCompletesSuccessfully) {
  RunGivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListTest(
      *this, true, false, false, false);
}

TEST_P(
    zeCommandListEventTestsExtended,
    GivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListThenCommandCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()),
      std::get<2>(GetParam()));
}

TEST_P(
    zeCommandListEventTestsExtended,
    GivenMemoryCopyRegionWithDependenciesWhenExecutingImmediateCommandListThenCommandCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()),
      std::get<2>(GetParam()));
}

void RunGivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate, bool is_shared_system_src,
    bool is_shared_system_dst) {
  // This test is similar to the previous except that there is an
  // added delay to specifically test the wait functionality

  uint32_t width = 16;
  uint32_t height = 16;
  test.size = height * width;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_src);
  auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      test.size, is_shared_system_dst);
  memset(src_buffer, 0x1, test.size);
  memset(dst_buffer, 0x0, test.size);

  // Verify Host Reads Event as unset
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventHostSynchronize(test.hEvent, 0));

  // Execute and verify Device reads event
  ze_copy_region_t sr = {0U, 0U, 0U, width, height, 0U};
  ze_copy_region_t dr = {0U, 0U, 0U, width, height, 0U};
  lzt::append_memory_copy_region(cmd_bundle.list, dst_buffer, &dr, width, 0,
                                 src_buffer, &sr, width, 0, nullptr, 1,
                                 &test.hEvent);
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Verify Copy Waits for Signal
  // This sleep simulates work (e.g. file i/o) on the host that would cause
  // with a high probability the device to have to wait
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Allocation can be accessed (while being used by GPU) only concurrently
  if (lzt::is_concurrent_memory_access_supported(
          lzt::get_default_device(lzt::get_default_driver()))) {
    EXPECT_NE(0, memcmp(src_buffer, dst_buffer, test.size));
  }

  // Signal Event On Host
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSignal(test.hEvent));
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  // Verify Memory Set completed
  EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, test.size));
  lzt::free_memory_with_allocator_selector(src_buffer, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(dst_buffer, is_shared_system_dst);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListTest(
      *this, false, false, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListTest(
      *this, true, false, false);
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListTest(
      *this, false, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

TEST_P(
    zeCommandListEventTests,
    GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListTest(
      *this, true, std::get<0>(GetParam()), std::get<1>(GetParam()));
}

static ze_image_handle_t create_test_image(int height, int width) {
  ze_image_desc_t image_description = {};
  image_description.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_description.format.layout = ZE_IMAGE_FORMAT_LAYOUT_32;

  image_description.pNext = nullptr;
  image_description.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  image_description.type = ZE_IMAGE_TYPE_2D;
  image_description.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  image_description.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_description.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_description.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_description.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_description.width = width;
  image_description.height = height;
  image_description.depth = 1;
  ze_image_handle_t image = nullptr;

  image = lzt::create_ze_image(image_description);

  return image;
}

void RunGivenImageCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  int width = input.width();
  int height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_event_handle_t hEvent1, hEvent2, hEvent3, hEvent4;
  test.ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent2, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent3, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent4, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmd_bundle.list, input_xeimage,
                                  input.raw_data(), hEvent1);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &hEvent1);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmd_bundle.list, output_xeimage, input_xeimage,
                         hEvent2);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &hEvent2);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmd_bundle.list, output_xeimage, input_xeimage,
                                &dr, &sr, hEvent3);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &hEvent3);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmd_bundle.list, output.raw_data(),
                                output_xeimage, hEvent4);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &hEvent4);
  // execute commands
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Make sure all events signaled from host perspective
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent1, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent2, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent3, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent4, UINT64_MAX));

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  // cleanup
  test.ep.destroy_event(hEvent1);
  test.ep.destroy_event(hEvent2);
  test.ep.destroy_event(hEvent3);
  test.ep.destroy_event(hEvent4);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenImageCopyThatSignalsEventWhenCompleteWhenExecutingCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenImageCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenImageCopyThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly) {
  RunGivenImageCopyThatSignalsEventWhenCompleteWhenExecutingCommandListTest(
      *this, true);
}

void RunGivenImageCopyThatWaitsOnEventWhenExecutingCommandListTest(
    zeCommandListEventTests &test, bool is_immediate) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  int width = input.width();
  int height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_event_handle_t hEvent0, hEvent1, hEvent2, hEvent3, hEvent4;
  test.ep.create_event(hEvent0, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent2, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent3, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  test.ep.create_event(hEvent4, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  lzt::append_signal_event(cmd_bundle.list, hEvent0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmd_bundle.list, input_xeimage,
                                  input.raw_data(), hEvent1, 1, &hEvent0);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmd_bundle.list, output_xeimage, input_xeimage,
                         hEvent2, 1, &hEvent1);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmd_bundle.list, output_xeimage, input_xeimage,
                                &dr, &sr, hEvent3, 1, &hEvent2);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmd_bundle.list, output.raw_data(),
                                output_xeimage, hEvent4, 1, &hEvent3);
  lzt::append_wait_on_events(cmd_bundle.list, 1, &hEvent4);
  // execute commands
  lzt::close_command_list(cmd_bundle.list);
  if (!is_immediate) {
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
  }

  // Make sure all events signaled from host perspective
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent0, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent1, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent2, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent3, UINT64_MAX));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(hEvent4, UINT64_MAX));

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  // cleanup
  test.ep.destroy_event(hEvent0);
  test.ep.destroy_event(hEvent1);
  test.ep.destroy_event(hEvent2);
  test.ep.destroy_event(hEvent3);
  test.ep.destroy_event(hEvent4);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListEventTests,
    GivenImageCopyThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenImageCopyThatWaitsOnEventWhenExecutingCommandListTest(*this, false);
}

TEST_F(
    zeCommandListEventTests,
    GivenImageCopyThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully) {
  RunGivenImageCopyThatWaitsOnEventWhenExecutingCommandListTest(*this, true);
}

void RunGivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesTest(
    bool is_immediate, bool is_shared_system_src, bool is_shared_system_temp,
    bool is_shared_system_dst) {
  const size_t size = 1024;
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_devices(driver);

  ASSERT_GT(devices.size(), 0);
  bool test_run = false;
  for (auto &device : devices) {
    auto command_queue_groups = lzt::get_command_queue_group_properties(device);

    // we want to test copies with differing engines
    std::vector<int> copy_ordinals;
    for (int i = 0; i < command_queue_groups.size(); i++) {
      if (command_queue_groups[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
        copy_ordinals.push_back(i);
      }
    }

    if (copy_ordinals.size() < 2) {
      continue;
    }
    test_run = true;

    auto cmd_bundle_0 = lzt::create_command_bundle(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, copy_ordinals[0], 0, is_immediate);
    auto cmd_bundle_1 = lzt::create_command_bundle(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, copy_ordinals[1], 0, is_immediate);

    ze_event_pool_desc_t event_pool_desc = {};
    event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    event_pool_desc.flags = 0;
    event_pool_desc.count = 1;
    auto event_pool = lzt::create_event_pool(context, event_pool_desc);
    ze_event_desc_t event_desc = {};
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.index = 0;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    auto event = lzt::create_event(event_pool, event_desc);

    auto src_buffer = lzt::allocate_shared_memory_with_allocator_selector(
        size, 1, 0, 0, device, context, is_shared_system_src);
    auto temp_buffer = lzt::allocate_shared_memory_with_allocator_selector(
        size, 1, 0, 0, device, context, is_shared_system_temp);
    auto dst_buffer = lzt::allocate_shared_memory_with_allocator_selector(
        size, 1, 0, 0, device, context, is_shared_system_dst);
    memset(src_buffer, 0x1, size);
    memset(temp_buffer, 0xFF, size);
    memset(dst_buffer, 0x0, size);

    // Execute and verify GPU reads event
    lzt::append_memory_copy(cmd_bundle_0.list, temp_buffer, src_buffer, size,
                            event, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle_1.list, dst_buffer, temp_buffer, size,
                            nullptr, 1, &event);

    lzt::close_command_list(cmd_bundle_0.list);
    lzt::close_command_list(cmd_bundle_1.list);
    if (is_immediate) {
      lzt::synchronize_command_list_host(cmd_bundle_0.list, UINT64_MAX);
      lzt::synchronize_command_list_host(cmd_bundle_1.list, UINT64_MAX);
    } else {

      lzt::execute_command_lists(cmd_bundle_1.queue, 1, &cmd_bundle_1.list,
                                 nullptr);
      lzt::execute_command_lists(cmd_bundle_0.queue, 1, &cmd_bundle_0.list,
                                 nullptr);

      lzt::synchronize(cmd_bundle_0.queue, UINT64_MAX);
      lzt::synchronize(cmd_bundle_1.queue, UINT64_MAX);
    }

    // Verify Memory Copy completed
    EXPECT_EQ(0, memcmp(src_buffer, dst_buffer, size));

    lzt::free_memory_with_allocator_selector(context, src_buffer,
                                             is_shared_system_src);
    lzt::free_memory_with_allocator_selector(context, temp_buffer,
                                             is_shared_system_temp);
    lzt::free_memory_with_allocator_selector(context, dst_buffer,
                                             is_shared_system_dst);
    lzt::destroy_event(event);
    lzt::destroy_event_pool(event_pool);
    lzt::destroy_command_bundle(cmd_bundle_0);
    lzt::destroy_command_bundle(cmd_bundle_1);
  }
  lzt::destroy_context(context);

  if (!test_run) {
    LOG_WARNING << "Less than 2 engines that support copy, test not run";
  }
}

TEST(
    zeCommandListCopyEventTest,
    GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesThenCopiesCompleteSuccessfully) {
  RunGivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesTest(
      false, false, false, false);
}

TEST(
    zeCommandListCopyEventTest,
    GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesOnImmediateCmdListsThenCopiesCompleteSuccessfully) {
  RunGivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesTest(
      true, false, false, false);
}

TEST(
    zeCommandListCopyEventTest,
    GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesThenCopiesCompleteSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  for (int i = 1; i < 8; ++i) {
    std::bitset<3> bits(i);
    RunGivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesTest(
        false, bits[2], bits[1], bits[0]);
  }
}

TEST(
    zeCommandListCopyEventTest,
    GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesOnImmediateCmdListsThenCopiesCompleteSuccessfullyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  for (int i = 1; i < 8; ++i) {
    std::bitset<3> bits(i);
    RunGivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesTest(
        true, bits[2], bits[1], bits[0]);
  }
}

INSTANTIATE_TEST_SUITE_P(ParametrizedTests, zeCommandListEventTests,
                         ::testing::Values(std::make_tuple(false, true),
                                           std::make_tuple(true, false),
                                           std::make_tuple(true, true)));

INSTANTIATE_TEST_SUITE_P(ParametrizedTests, zeCommandListEventTestsExtended,
                         ::testing::Values(std::make_tuple(false, false, true),
                                           std::make_tuple(false, true, false),
                                           std::make_tuple(false, true, true),
                                           std::make_tuple(true, false, false),
                                           std::make_tuple(true, false, true),
                                           std::make_tuple(true, true, false),
                                           std::make_tuple(true, true, true)));

} // namespace
