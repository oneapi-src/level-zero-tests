/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

#include <algorithm>
#include <cstdlib>
#include <array>
#include <string>

using namespace level_zero_tests;

namespace {

using lzt::to_int;
using lzt::to_u32;
using lzt::to_u8;

class zeCommandListAppendMemoryFillTests : public ::testing::Test {
protected:
  template <lzt::command_list_mode_t Mode>
  void RunMaxMemoryFillTest(bool is_shared_system, bool use_madvise);
  template <lzt::command_list_mode_t Mode>
  void RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest(
      bool is_shared_system, bool use_copy_engine, bool use_madvise);
  template <lzt::command_list_mode_t Mode>
  void RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(
      bool is_shared_system, bool use_copy_engine, bool use_madvise);
  template <lzt::command_list_mode_t Mode>
  void RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
      bool is_shared_system, bool use_copy_engine, bool use_madvise);

  void SetUp() override {
    device = zeDevice::get_instance()->get_device();
    cmd_queue_group_props = get_command_queue_group_properties(device);
  }

  ze_device_handle_t device;
  std::vector<ze_command_queue_group_properties_t> cmd_queue_group_props;
};

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillTests::
    RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest(bool is_shared_system,
                                                          bool use_copy_engine,
                                                          bool use_madvise) {
  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);
  const size_t size = 4096;
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size, is_shared_system);
  const uint8_t pattern = 0x00;
  const int pattern_size = 1;

  if (is_shared_system && use_madvise) {
    lzt::append_memory_advise(
        cmd_bundle.list, device, memory, size,
        ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
  }

  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          nullptr);

  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::regular>(true, true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillTest<
      lzt::command_list_mode_t::immediate>(true, true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillTests::RunMaxMemoryFillTest(
    bool is_shared_system, bool use_madvise) {
  auto device_properties = lzt::get_device_properties(device);
  auto max_alloc_memsize = device_properties.maxMemAllocSize;

  for (size_t i = 0U; i < cmd_queue_group_props.size(); i++) {
    auto bundle = lzt::create_command_bundle<Mode>(lzt::get_default_context(),
                                                   device, 0u, to_u32(i));
    size_t size = cmd_queue_group_props[i].maxMemoryFillPatternSize;
    size_t pattern_size = cmd_queue_group_props[i].maxMemoryFillPatternSize;
    if (cmd_queue_group_props[i].maxMemoryFillPatternSize > max_alloc_memsize) {
      pattern_size = 1;
      size = 4096;
    }
    void *memory = lzt::allocate_device_memory_with_allocator_selector(
        size, is_shared_system);
    const uint8_t pattern = 0x00;

    if (is_shared_system && use_madvise) {
      lzt::append_memory_advise(
          bundle.list, device, memory, size,
          ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
    }

    lzt::append_memory_fill(bundle.list, memory, &pattern, pattern_size, size,
                            nullptr);

    execute_and_sync_command_bundle(bundle, UINT64_MAX);
    reset_command_list(bundle.list);
    lzt::free_memory_with_allocator_selector(memory, is_shared_system);
    lzt::destroy_command_bundle(bundle);
  }
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunMaxMemoryFillTest<lzt::command_list_mode_t::regular>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenImmediateCommandListAndMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunMaxMemoryFillTest<lzt::command_list_mode_t::immediate>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunMaxMemoryFillTest<lzt::command_list_mode_t::regular>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenImmediateCommandListAndMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunMaxMemoryFillTest<lzt::command_list_mode_t::immediate>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenMaxMemoryFillPatternSizeAndMemAdviceForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunMaxMemoryFillTest<lzt::command_list_mode_t::regular>(true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenImmediateCommandListMaxMemoryFillPatternSizeAndMemAdviceForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunMaxMemoryFillTest<lzt::command_list_mode_t::immediate>(true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillTests::
    RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(
        bool is_shared_system, bool use_copy_engine, bool use_madvise) {
  lzt::zeEventPool ep;

  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);
  const size_t size = 4096;
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size, is_shared_system);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  if (is_shared_system && use_madvise) {
    lzt::append_memory_advise(
        cmd_bundle.list, device, memory, size,
        ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
  }

  ep.create_event(hEvent);
  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          hEvent);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::regular>(true, true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillTests::
    RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
        bool is_shared_system, bool use_copy_engine, bool use_madvise) {
  lzt::zeEventPool ep;

  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);
  const size_t size = 4096;
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size, is_shared_system);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  if (is_shared_system && use_madvise) {
    lzt::append_memory_advise(
        cmd_bundle.list, device, memory, size,
        ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
  }

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(false, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(false, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenAdvisedSharedSystemMemorySizeValueAndCopyEngineWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, true, true);
}

class zeCommandListAppendMemoryFillVerificationTests : public ::testing::Test {
protected:
  template <lzt::command_list_mode_t Mode>
  void RunGivenHostMemoryWhenExecutingAMemoryFillTest();
  template <lzt::command_list_mode_t Mode>
  void RunGivenSharedMemoryWhenExecutingAMemoryFillTest();
  template <lzt::command_list_mode_t Mode>
  void RunGivenDeviceMemoryWhenExecutingAMemoryFillTest();
  template <lzt::command_list_mode_t Mode>
  void RunGivenSharedSystemMemoryWhenExecutingAMemoryFillTest();
};

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenHostMemoryWhenExecutingAMemoryFillTest() {
  size_t size = 16;
  auto memory = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

LZT_TEST_F(zeCommandListAppendMemoryFillVerificationTests,
           GivenHostMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenHostMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenHostMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenHostMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::immediate>();
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenSharedMemoryWhenExecutingAMemoryFillTest() {
  size_t size = 16;
  auto memory = allocate_shared_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

LZT_TEST_F(zeCommandListAppendMemoryFillVerificationTests,
           GivenSharedMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenSharedMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenSharedMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenSharedMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::immediate>();
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenDeviceMemoryWhenExecutingAMemoryFillTest() {
  size_t size = 16;
  auto memory = allocate_device_memory(size);
  auto local_mem = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListReset(cmd_bundle.list));
  append_memory_copy(cmd_bundle.list, local_mem, memory, size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(local_mem)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
  free_memory(local_mem);
}

LZT_TEST_F(zeCommandListAppendMemoryFillVerificationTests,
           GivenDeviceMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenDeviceMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenDeviceMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenDeviceMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::immediate>();
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenSharedSystemMemoryWhenExecutingAMemoryFillTest() {
  size_t size = 16;
  auto memory = malloc(size);
  ASSERT_NE(nullptr, memory);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free(memory);
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenSharedSystemMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenSharedSystemMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenSharedSystemMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenSharedSystemMemoryWhenExecutingAMemoryFillTest<
      lzt::command_list_mode_t::immediate>();
}

class zeCommandListAppendMemoryFillSubDeviceVerificationTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_type_t, lzt::command_list_mode_t>> {};
LZT_TEST_P(
    zeCommandListAppendMemoryFillSubDeviceVerificationTests,
    GivenMemoryAllocationWhenExecutingAMemoryFillWithSubDeviceThenMemoryIsSetCorrectly) {

  auto memory_type = std::get<0>(GetParam());
  lzt::command_list_mode_t mode = std::get<1>(GetParam());
  uint32_t test_count = 0;
  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  const size_t size = 16;
  void *host_memory = lzt::allocate_host_memory(size, 1, context);

  for (auto device : devices) {
    auto subdevices = lzt::get_ze_sub_devices(device);

    if (subdevices.empty()) {
      continue;
    }
    test_count++;
    for (auto subdevice : subdevices) {
      auto cmd_bundle = lzt::create_command_bundle(subdevice, mode);

      void *memory = nullptr;
      switch (memory_type) {
      case ZE_MEMORY_TYPE_DEVICE:
        memory = lzt::allocate_device_memory(size, 1, 0, device, context);
        break;
      case ZE_MEMORY_TYPE_HOST:
        memory = lzt::allocate_host_memory(size, 1, context);
        break;
      case ZE_MEMORY_TYPE_SHARED:
        memory = lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
        break;
      default:
        LOG_WARNING << "Unhandled memory type for memory fill subdevice test: "
                    << memory_type;
      }

      uint8_t pattern = 0xAB;
      const int pattern_size = 1;

      lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size,
                              size, nullptr);
      lzt::append_barrier(cmd_bundle.list);
      lzt::append_memory_copy(cmd_bundle.list, host_memory, memory, size);
      lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

      for (size_t i = 0U; i < size; i++) {
        ASSERT_EQ(static_cast<uint8_t *>(host_memory)[i], pattern)
            << "Memory Fill did not match on sub-device";
      }

      lzt::free_memory(context, memory);
      lzt::destroy_command_bundle(cmd_bundle);
    }
  }
  lzt::free_memory(context, host_memory);

  if (!test_count) {
    LOG_WARNING << "No Sub-Device Memory Fill tests run";
  }
}

INSTANTIATE_TEST_SUITE_P(
    SubDeviceMemoryFills,
    zeCommandListAppendMemoryFillSubDeviceVerificationTests,
    ::testing::Combine(::testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                         ZE_MEMORY_TYPE_HOST,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

class zeCommandListAppendMemoryFillPatternVerificationTests
    : public zeCommandListAppendMemoryFillVerificationTests,
      public ::testing::WithParamInterface<
          std::tuple<size_t, lzt::command_list_mode_t>> {
protected:
  void RunGivenPatternSizeWhenExecutingAMemoryFillTest(bool is_shared_system);
};

void zeCommandListAppendMemoryFillPatternVerificationTests::
    RunGivenPatternSizeWhenExecutingAMemoryFillTest(bool is_shared_system) {
  const size_t pattern_size = std::get<0>(GetParam());
  lzt::command_list_mode_t mode = std::get<1>(GetParam());
  auto cmd_bundle = lzt::create_command_bundle(mode);
  const size_t total_size = (pattern_size * 10) + 5;
  auto pattern = std::make_unique<uint8_t[]>(pattern_size);
  auto target_memory = lzt::allocate_host_memory_with_allocator_selector(
      total_size, is_shared_system);

  for (size_t i = 0U; i < pattern_size; i++) {
    pattern[i] = to_u8(i);
  }

  append_memory_fill(cmd_bundle.list, target_memory, pattern.get(),
                     pattern_size, total_size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (size_t i = 0U; i < total_size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(target_memory)[i], i % pattern_size)
        << "Memory Fill did not match.";
  }
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(target_memory, is_shared_system);
}

LZT_TEST_P(zeCommandListAppendMemoryFillPatternVerificationTests,
           GivenPatternSizeWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenPatternSizeWhenExecutingAMemoryFillTest(false);
}

LZT_TEST_P(
    zeCommandListAppendMemoryFillPatternVerificationTests,
    GivenPatternSizeWhenExecutingAMemoryFillThenMemoryIsSetCorrectlyWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenPatternSizeWhenExecutingAMemoryFillTest(true);
}

INSTANTIATE_TEST_SUITE_P(
    VaryPatternSize, zeCommandListAppendMemoryFillPatternVerificationTests,
    ::testing::Combine(::testing::Values(1, 2, 4, 8, 16, 32, 64, 128),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

class zeCommandListCommandQueueTests : public ::testing::Test {};

class zeCommandListAppendMemoryCopyWithDataVerificationTests
    : public zeCommandListCommandQueueTests {
protected:
  template <lzt::command_list_mode_t Mode>
  void RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest();
  template <lzt::command_list_mode_t Mode>
  void RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest();
};

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyWithDataVerificationTests::
    RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest() {
  const size_t size = 4 * 1024;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(size_in_bytes(host_memory1));
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  append_memory_copy(cmd_bundle.list, device_memory, host_memory1.data(),
                     size_in_bytes(host_memory1), nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(cmd_bundle.list, host_memory2.data(), device_memory,
                     size_in_bytes(host_memory2), nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  free_memory(device_memory);
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>();
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyWithDataVerificationTests::
    RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest() {
  const size_t size = 1024;
  // number of transfers to device memory
  const size_t num_dev_mem_copy = 8;
  // byte address alignment
  const size_t addr_alignment = 1;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(
      num_dev_mem_copy * (size_in_bytes(host_memory1) + addr_alignment));
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  uint8_t *char_src_ptr = static_cast<uint8_t *>(device_memory);
  uint8_t *char_dst_ptr = char_src_ptr;
  append_memory_copy(cmd_bundle.list, static_cast<void *>(char_dst_ptr),
                     host_memory1.data(), size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  for (uint32_t i = 1; i < num_dev_mem_copy; i++) {
    char_src_ptr = char_dst_ptr;
    char_dst_ptr += (size + addr_alignment);
    append_memory_copy(cmd_bundle.list, static_cast<void *>(char_dst_ptr),
                       static_cast<void *>(char_src_ptr), size, nullptr);
    append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  }
  append_memory_copy(cmd_bundle.list, host_memory2.data(),
                     static_cast<void *>(char_dst_ptr), size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);

  free_memory(device_memory);
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>();
}

enum class MemoryHint { None, AdviceToSystem, Prefetch };

class zeCommandListAppendMemoryBackToBackTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          bool, bool, lzt::command_list_mode_t, bool, MemoryHint, size_t>> {

protected:
  void RunGivenCopyBetweenUsmAndSharedSystemUsmAndVerifyCorrect(
      bool is_src_shared_system, bool is_dst_shared_system,
      lzt::command_list_mode_t mode, bool is_copy_only, MemoryHint memory_hint,
      size_t size) {
    const uint8_t value = to_u8(rand()) & 0xFF;
    const uint8_t init = (value + 0x22) & 0xFF;

    void *src_memory = lzt::allocate_shared_memory_with_allocator_selector(
        size, is_src_shared_system);
    void *dst_memory = lzt::allocate_shared_memory_with_allocator_selector(
        size, is_dst_shared_system);

    uint32_t ordinal = 0;
    if (is_copy_only) {
      ordinal = 1;
    }

    const char *src_memory_type = is_src_shared_system ? "SVM" : "USM";
    const char *dst_memory_type = is_dst_shared_system ? "SVM" : "USM";

    auto memory_hint_to_str = [](MemoryHint memory_hint) {
      switch (memory_hint) {
      case MemoryHint::None:
        return "None";
      case MemoryHint::AdviceToSystem:
        return "AdviseToSystem";
      case MemoryHint::Prefetch:
        return "Prefetch";
      }
      return "";
    };

    LOG_INFO << "src_memory (" << src_memory_type << ")= " << src_memory
             << " dst_memory (" << dst_memory_type << ")= " << dst_memory
             << " immediate = " << (mode != lzt::command_list_mode_t::regular)
             << " is_copy_only = " << is_copy_only
             << " memory_hint = " << memory_hint_to_str(memory_hint)
             << " size = " << size;

    memset(src_memory, value, size);
    memset(dst_memory, init, size);

    auto device = zeDevice::get_instance()->get_device();
    auto cmd_bundle = lzt::create_command_bundle(
        lzt::get_default_context(), device, 0u,
        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
        0u, ordinal, 0u, mode);

    if (is_src_shared_system) {
      switch (memory_hint) {
      case MemoryHint::AdviceToSystem:
        lzt::append_memory_advise(
            cmd_bundle.list, device, src_memory, size,
            ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
        break;
      case MemoryHint::Prefetch:
        lzt::append_memory_prefetch(cmd_bundle.list, src_memory, size);
        break;
      }
    }

    if (is_dst_shared_system) {
      switch (memory_hint) {
      case MemoryHint::AdviceToSystem:
        lzt::append_memory_advise(
            cmd_bundle.list, device, dst_memory, size,
            ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
        break;
      case MemoryHint::Prefetch:
        lzt::append_memory_prefetch(cmd_bundle.list, dst_memory, size);
        break;
      }
    }

    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<void *>(src_memory), size);

    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
    EXPECT_EQ(0, memcmp(src_memory, dst_memory, size));

    free_memory_with_allocator_selector(src_memory, is_src_shared_system);
    free_memory_with_allocator_selector(dst_memory, is_dst_shared_system);
  }
};

LZT_TEST_P(
    zeCommandListAppendMemoryBackToBackTests,
    GivenAllUsmAndSvmPermutationsConfirmSuccessfulExecutionBackToBackWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  bool is_src_shared_system = std::get<0>(GetParam());
  bool is_dst_shared_system = std::get<1>(GetParam());
  MemoryHint memory_hint = std::get<4>(GetParam());
  if (memory_hint != MemoryHint::None && !is_src_shared_system &&
      !is_dst_shared_system) {
    GTEST_SKIP() << "Skipping redundant cases.";
  }
  RunGivenCopyBetweenUsmAndSharedSystemUsmAndVerifyCorrect(
      is_src_shared_system, is_dst_shared_system, std::get<2>(GetParam()),
      std::get<3>(GetParam()), memory_hint, std::get<5>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    ParameterizedTests, zeCommandListAppendMemoryBackToBackTests,
    ::testing::Combine(::testing::Bool(), ::testing::Bool(),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate),
                       ::testing::Bool(),
                       ::testing::Values(MemoryHint::None,
                                         MemoryHint::AdviceToSystem,
                                         MemoryHint::Prefetch),
                       ::testing::Values(10, 10 * 1024, 32 * 1024 * 1024)));

class zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests
    : public zeCommandListCommandQueueTests {
protected:
  template <lzt::command_list_mode_t Mode>
  void
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest();
  template <lzt::command_list_mode_t Mode>
  void
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest();
};

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests::
    RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest() {
  const size_t size = 4 * 1024;
  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();

  EXPECT_NE(src_context, dflt_context);

  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);
  void *shared_memory_src_ctx = lzt::allocate_shared_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *shared_memory_dflt_ctx = lzt::allocate_shared_memory(size);
  void *device_memory_src_ctx = lzt::allocate_device_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *device_memory_dflt_ctx = lzt::allocate_device_memory(size);
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  lzt::write_data_pattern(host_memory_src_ctx, size, 1);

  append_memory_copy(src_context, cmd_bundle.list, device_memory_dflt_ctx,
                     host_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(dflt_context, cmd_bundle.list, device_memory_src_ctx,
                     device_memory_dflt_ctx, size, nullptr, 0, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(src_context, cmd_bundle.list, shared_memory_dflt_ctx,
                     device_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(dflt_context, cmd_bundle.list, shared_memory_src_ctx,
                     device_memory_dflt_ctx, size, nullptr, 0, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(src_context, cmd_bundle.list, host_memory_dflt_ctx,
                     shared_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory_dflt_ctx, size, 1);
  free_memory(src_context, host_memory_src_ctx);
  free_memory(host_memory_dflt_ctx);
  free_memory(src_context, shared_memory_src_ctx);
  free_memory(shared_memory_dflt_ctx);
  free_memory(src_context, device_memory_src_ctx);
  free_memory(device_memory_dflt_ctx);
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>();
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests::
    RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest() {
  const size_t size = 4 * 1024;
  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();
  EXPECT_NE(src_context, dflt_context);
  ze_event_handle_t hEvent1 = nullptr;
  ze_event_handle_t hEvent2 = nullptr;
  lzt::zeEventPool ep;
  ep.create_event(hEvent1);
  ep.create_event(hEvent2);

  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);
  void *device_memory_src_ctx = lzt::allocate_device_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *device_memory_dflt_ctx = lzt::allocate_device_memory(size);
  auto cmd_bundle = lzt::create_command_bundle<Mode>();

  lzt::write_data_pattern(host_memory_src_ctx, size, 1);

  append_memory_copy(src_context, cmd_bundle.list, device_memory_dflt_ctx,
                     host_memory_src_ctx, size, hEvent1, 0, nullptr);
  append_barrier(cmd_bundle.list, hEvent2, 1, &hEvent1);
  append_memory_copy(dflt_context, cmd_bundle.list, device_memory_src_ctx,
                     device_memory_dflt_ctx, size, hEvent1, 1, &hEvent2);
  append_barrier(cmd_bundle.list, hEvent2, 1, &hEvent1);
  append_memory_copy(src_context, cmd_bundle.list, host_memory_dflt_ctx,
                     device_memory_src_ctx, size, hEvent1, 0, &hEvent2);
  append_barrier(cmd_bundle.list, nullptr, 1, &hEvent1);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory_dflt_ctx, size, 1);
  ep.destroy_event(hEvent1);
  ep.destroy_event(hEvent2);
  free_memory(src_context, host_memory_src_ctx);
  free_memory(host_memory_dflt_ctx);
  free_memory(src_context, device_memory_src_ctx);
  free_memory(device_memory_dflt_ctx);
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest<
      lzt::command_list_mode_t::immediate>();
}

class zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, size_t, size_t, ze_memory_type_t, ze_memory_type_t,
                     lzt::command_list_mode_t>> {
protected:
  zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests() {
    rows = std::get<0>(GetParam());
    columns = std::get<1>(GetParam());
    slices = std::get<2>(GetParam());

    memory_size = rows * columns * slices;

    ze_memory_type_t source_type = std::get<3>(GetParam());
    ze_memory_type_t destination_type = std::get<4>(GetParam());

    mode = std::get<5>(GetParam());
    cmd_bundle = lzt::create_command_bundle(mode);

    switch (source_type) {
    case ZE_MEMORY_TYPE_DEVICE:
      source_memory = allocate_device_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_HOST:
      source_memory = allocate_host_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_SHARED:
      source_memory = allocate_shared_memory(memory_size);
      break;
    default:
      abort();
    }

    switch (destination_type) {
    case ZE_MEMORY_TYPE_DEVICE:
      destination_memory = allocate_device_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_HOST:
      destination_memory = allocate_host_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_SHARED:
      destination_memory = allocate_shared_memory(memory_size);
      break;
    default:
      abort();
    }

    // Set up memory buffers for test
    // Device memory has to be copied in so
    // use temporary buffers
    temp_src = allocate_host_memory(memory_size);
    temp_dest = allocate_host_memory(memory_size);

    memset(temp_dest, 0, memory_size);
    write_data_pattern(temp_src, memory_size, 1);

    append_memory_copy(cmd_bundle.list, destination_memory, temp_dest,
                       memory_size);
    append_memory_copy(cmd_bundle.list, source_memory, temp_src, memory_size);
    append_barrier(cmd_bundle.list);
  }

  ~zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests() {
    destroy_command_bundle(cmd_bundle);

    free_memory(source_memory);
    free_memory(destination_memory);
    free_memory(temp_src);
    free_memory(temp_dest);
  }

  void test_copy_region() {
    void *verification_memory = allocate_host_memory(memory_size);

    uint32_t wdth = to_u32(columns);
    uint32_t hght = to_u32(rows);
    uint32_t dpth = to_u32(slices);
    std::array<uint32_t, 3> widths = {1, wdth / 2, wdth};
    std::array<uint32_t, 3> heights = {1, hght / 2, hght};
    std::array<uint32_t, 3> depths = {1, dpth / 2, dpth};

    for (uint32_t region = 0; region < 3; region++) {
      // Define region to be copied from/to
      uint32_t width = widths[region];
      uint32_t height = heights[region];
      uint32_t depth = depths[region];

      ze_copy_region_t src_region;
      src_region.originX = 0;
      src_region.originY = 0;
      src_region.originZ = 0;
      src_region.width = width;
      src_region.height = height;
      src_region.depth = depth;

      ze_copy_region_t dest_region;
      dest_region.originX = 0;
      dest_region.originY = 0;
      dest_region.originZ = 0;
      dest_region.width = width;
      dest_region.height = height;
      dest_region.depth = depth;

      append_memory_copy_region(cmd_bundle.list, destination_memory,
                                &dest_region, wdth, wdth * hght, source_memory,
                                &src_region, wdth, wdth * hght, nullptr);
      append_barrier(cmd_bundle.list);
      append_memory_copy(cmd_bundle.list, verification_memory,
                         destination_memory, memory_size);
      execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
      reset_command_list(cmd_bundle.list);

      for (size_t z = 0U; z < depth; z++) {
        for (size_t y = 0U; y < height; y++) {
          for (size_t x = 0U; x < width; x++) {
            // index calculated based on memory sized by rows * columns * slices
            size_t index = z * columns * rows + y * columns + x;
            uint8_t dest_val =
                static_cast<uint8_t *>(verification_memory)[index];
            uint8_t src_val = static_cast<uint8_t *>(temp_src)[index];

            ASSERT_EQ(dest_val, src_val)
                << "Copy failed with region(w,h,d)=(" << width << ", " << height
                << ", " << depth << ")";
          }
        }
      }
    }
  }

  lzt::command_bundle cmd_bundle;

  void *source_memory, *temp_src;
  void *destination_memory, *temp_dest;

  size_t rows;
  size_t columns;
  size_t slices;
  lzt::command_list_mode_t mode;
  size_t memory_size;
};

LZT_TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests,
    GivenValidMemoryWhenAppendingMemoryCopyWithRegionThenSuccessIsReturnedAndCopyIsCorrect) {
  test_copy_region();
}

auto memory_types = ::testing::Values(
    ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_HOST, ZE_MEMORY_TYPE_SHARED);
INSTANTIATE_TEST_SUITE_P(
    MemoryCopies,
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests,
    ::testing::Combine(::testing::Values(8, 64),    // Rows
                       ::testing::Values(8, 64),    // Cols
                       ::testing::Values(1, 8, 64), // Slices
                       memory_types,                // Source Memory Type
                       memory_types,                // Destination Memory Type
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));
class AppendMemoryCopyRegionWithSharedSystem
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, size_t, size_t, lzt::command_list_mode_t, bool>> {
protected:
  enum class MemoryHint {
    None,
    AdviseSourceToSystem,
    AdviseDestinationToSystem,
    AdviseBothToSystem,
    PrefetchSource,
    PrefetchDestination,
    PrefetchBoth
  };

  void RunMemoryCopyRegionTest(MemoryHint memory_hint) {

    mode = std::get<3>(GetParam());
    use_copy_engine = std::get<4>(GetParam());

    auto device = zeDevice::get_instance()->get_device();
    auto cmd_queue_group_props = get_command_queue_group_properties(device);

    const ze_command_queue_group_property_flags_t queue_group_flags_set =
        use_copy_engine
            ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
            : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
               ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
    const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
        use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
    auto ordinal =
        lzt::get_queue_ordinal(cmd_queue_group_props, queue_group_flags_set,
                               queue_group_flags_not_set);
    ASSERT_NE(ordinal, std::nullopt);

    auto cmd_bundle = lzt::create_command_bundle(lzt::get_default_context(),
                                                 device, 0u, *ordinal, mode);

    // Set up memory buffers for test
    // Device memory has to be copied in so
    // use temporary buffers
    temp_src = allocate_host_memory(memory_size);
    temp_dest = allocate_host_memory(memory_size);

    memset(temp_dest, 0, memory_size);
    write_data_pattern(temp_src, memory_size, 1);

    append_memory_copy(cmd_bundle.list, destination_memory, temp_dest,
                       memory_size);
    append_memory_copy(cmd_bundle.list, source_memory, temp_src, memory_size);
    append_barrier(cmd_bundle.list);

    void *verification_memory = allocate_host_memory(memory_size);

    uint32_t wdth = to_u32(columns);
    uint32_t hght = to_u32(rows);
    uint32_t dpth = to_u32(slices);
    std::array<uint32_t, 3> widths = {1, wdth / 2, wdth};
    std::array<uint32_t, 3> heights = {1, hght / 2, hght};
    std::array<uint32_t, 3> depths = {1, dpth / 2, dpth};

    for (uint32_t region = 0; region < 3; region++) {
      if (memory_hint == MemoryHint::AdviseSourceToSystem ||
          memory_hint == MemoryHint::AdviseBothToSystem) {
        lzt::append_memory_advise(
            cmd_bundle.list, device, source_memory, memory_size,
            ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
      }

      if (memory_hint == MemoryHint::AdviseDestinationToSystem ||
          memory_hint == MemoryHint::AdviseBothToSystem) {
        lzt::append_memory_advise(
            cmd_bundle.list, device, destination_memory, memory_size,
            ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION);
      }

      if (memory_hint == MemoryHint::PrefetchSource ||
          memory_hint == MemoryHint::PrefetchBoth) {
        lzt::append_memory_prefetch(cmd_bundle.list, source_memory,
                                    memory_size);
      }

      if (memory_hint == MemoryHint::PrefetchDestination ||
          memory_hint == MemoryHint::PrefetchBoth) {
        lzt::append_memory_prefetch(cmd_bundle.list, destination_memory,
                                    memory_size);
      }

      // Define region to be copied from/to
      uint32_t width = widths[region];
      uint32_t height = heights[region];
      uint32_t depth = depths[region];

      ze_copy_region_t src_region;
      src_region.originX = 0;
      src_region.originY = 0;
      src_region.originZ = 0;
      src_region.width = width;
      src_region.height = height;
      src_region.depth = depth;

      ze_copy_region_t dest_region;
      dest_region.originX = 0;
      dest_region.originY = 0;
      dest_region.originZ = 0;
      dest_region.width = width;
      dest_region.height = height;
      dest_region.depth = depth;

      append_memory_copy_region(cmd_bundle.list, destination_memory,
                                &dest_region, wdth, wdth * hght, source_memory,
                                &src_region, wdth, wdth * hght, nullptr);
      append_barrier(cmd_bundle.list);
      append_memory_copy(cmd_bundle.list, verification_memory,
                         destination_memory, memory_size);
      execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
      reset_command_list(cmd_bundle.list);

      for (size_t z = 0U; z < depth; z++) {
        for (size_t y = 0U; y < height; y++) {
          for (size_t x = 0U; x < width; x++) {
            // index calculated based on memory sized by rows * columns * slices
            size_t index = z * columns * rows + y * columns + x;
            uint8_t dest_val =
                static_cast<uint8_t *>(verification_memory)[index];
            uint8_t src_val = static_cast<uint8_t *>(temp_src)[index];

            ASSERT_EQ(dest_val, src_val)
                << "Copy failed with region(w,h,d)=(" << width << ", " << height
                << ", " << depth << ")";
          }
        }
      }
    }

    destroy_command_bundle(cmd_bundle);
    free_memory(temp_src);
    free_memory(temp_dest);
    free_memory(verification_memory);
  }

  lzt::command_bundle cmd_bundle;

  void *source_memory, *temp_src;
  void *destination_memory, *temp_dest;

  size_t rows;
  size_t columns;
  size_t slices;
  lzt::command_list_mode_t mode;
  bool use_copy_engine;
  size_t memory_size;
};

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToHostMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_host_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidAdvisedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToHostMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_host_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::AdviseSourceToSystem);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidPrefetchedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToHostMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_host_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchSource);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_shared_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidAdvisedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_shared_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::AdviseSourceToSystem);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidPrefetchedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_shared_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchSource);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToDeviceMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_device_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidAdvisedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToDeviceMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_device_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::AdviseSourceToSystem);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidPrefetchedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToDeviceMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_device_memory(memory_size);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchSource);
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidAdvisedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseSourceToSystem);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidPrefetchedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchSource);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToAdvisedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseDestinationToSystem);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToPrefetchedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchDestination);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidAdvisedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToAdvisedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseBothToSystem);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidPrefetchedSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToPrefetchedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchBoth);
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidHostMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_host_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidHostMemoryWhenAppendingMemoryCopyWithRegionToAdvisedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_host_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseDestinationToSystem);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidHostMemoryWhenAppendingMemoryCopyWithRegionToPrefetchedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_host_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchDestination);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_shared_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedMemoryWhenAppendingMemoryCopyWithRegionToAdvisedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_shared_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseDestinationToSystem);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidSharedMemoryWhenAppendingMemoryCopyWithRegionToPrefetchedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_shared_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchDestination);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidDeviceMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_device_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::None);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidDeviceMemoryWhenAppendingMemoryCopyWithRegionToAdvisedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_device_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::AdviseDestinationToSystem);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

LZT_TEST_P(
    AppendMemoryCopyRegionWithSharedSystem,
    GivenValidDeviceMemoryWhenAppendingMemoryCopyWithRegionToPrefetchedSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_device_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest(MemoryHint::PrefetchDestination);
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

INSTANTIATE_TEST_SUITE_P(
    MemoryCopies, AppendMemoryCopyRegionWithSharedSystem,
    ::testing::Combine(::testing::Values(8, 64), // Rows
                       ::testing::Values(8, 64), // Cols
                       ::testing::Values(1, 8,
                                         64), // Slices
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate),
                       ::testing::Bool()));

class zeCommandListAppendMemoryCopyTests : public ::testing::Test {
protected:
  template <lzt::command_list_mode_t Mode>
  void
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(bool is_shared_system,
                                                       bool use_copy_engine);
  template <lzt::command_list_mode_t Mode>
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(
      bool is_shared_system, bool use_copy_engine);
  template <lzt::command_list_mode_t Mode>
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(
      bool is_shared_system, bool use_copy_engine);
  template <lzt::command_list_mode_t Mode>
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
      bool is_shared_system);
};

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(bool is_shared_system,
                                                         bool use_copy_engine) {
  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);

  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);

  lzt::append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                          size_in_bytes(host_memory), nullptr);
  lzt::execute_and_sync_command_bundle(cmd_bundle,
                                       std::numeric_limits<uint64_t>().max());
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::regular>(true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest<
      lzt::command_list_mode_t::immediate>(true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(
        bool is_shared_system, bool use_copy_engine) {
  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);

  lzt::zeEventPool ep;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  lzt::append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                          size_in_bytes(host_memory), hEvent);
  lzt::execute_and_sync_command_bundle(cmd_bundle,
                                       std::numeric_limits<uint64_t>().max());
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::regular>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::immediate>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::regular>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyWithHEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::regular>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::immediate>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::regular>(true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest<
      lzt::command_list_mode_t::immediate>(true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(
        bool is_shared_system, bool use_copy_engine) {
  auto cmd_queue_group_props = get_command_queue_group_properties(
      zeDevice::get_instance()->get_device());

  const ze_command_queue_group_property_flags_t queue_group_flags_set =
      use_copy_engine
          ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY
          : (ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE |
             ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS);
  const ze_command_queue_group_property_flags_t queue_group_flags_not_set =
      use_copy_engine ? ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE : 0;
  auto ordinal = lzt::get_queue_ordinal(
      cmd_queue_group_props, queue_group_flags_set, queue_group_flags_not_set);
  ASSERT_NE(ordinal, std::nullopt);

  auto cmd_bundle = lzt::create_command_bundle<Mode>(
      lzt::get_default_context(), zeDevice::get_instance()->get_device(), 0u,
      *ordinal);

  lzt::zeEventPool ep;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  lzt::append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                          size_in_bytes(host_memory), nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  lzt::execute_and_sync_command_bundle(cmd_bundle,
                                       std::numeric_limits<uint64_t>().max());
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::regular>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(false, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::regular>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(false, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true, true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemorySizeAndCopyEngineWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true, true);
}

template <lzt::command_list_mode_t Mode>
void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
        bool is_shared_system) {

  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle<Mode>();
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;
  ze_copy_region_t dstRegion = {};
  ze_copy_region_t srcRegion = {};
  dstRegion.width = size;
  dstRegion.height = 1;
  dstRegion.depth = 1;
  srcRegion.width = size;
  srcRegion.height = 1;
  srcRegion.depth = 1;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  lzt::append_memory_copy_region(cmd_bundle.list, memory, &dstRegion, 0, 0,
                                 host_memory.data(), &srcRegion, 0, 0, nullptr,
                                 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  lzt::execute_and_sync_command_bundle(cmd_bundle,
                                       std::numeric_limits<uint64_t>().max());
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest<
      lzt::command_list_mode_t::regular>(false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionToSharedSystemMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest<
      lzt::command_list_mode_t::regular>(true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionToSharedSystemMemoryWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest<
      lzt::command_list_mode_t::immediate>(true);
}

class zeCommandListAppendMemoryPrefetchTests : public ::testing::Test {
protected:
  template <lzt::command_list_mode_t Mode>
  void RunGivenMemoryPrefetchWhenWritingFromDeviceTest(bool is_shared_system) {
    const size_t size = 16;
    const uint8_t value = 0x55;
    void *memory = lzt::allocate_shared_memory_with_allocator_selector(
        size, is_shared_system);
    memset(memory, 0xaa, size);
    auto cmd_bundle = lzt::create_command_bundle<Mode>();

    lzt::append_memory_prefetch(cmd_bundle.list, memory, size);
    lzt::append_memory_set(cmd_bundle.list, memory, &value, size);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(value, ((uint8_t *)memory)[i]);
    }

    free_memory_with_allocator_selector(memory, is_shared_system);
    lzt::destroy_command_bundle(cmd_bundle);
  }
};

LZT_TEST_F(zeCommandListAppendMemoryPrefetchTests,
           GivenMemoryPrefetchWhenWritingFromDeviceThenDataisCorrectFromHost) {
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest<
      lzt::command_list_mode_t::regular>(false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryPrefetchTests,
    GivenMemoryPrefetchWhenWritingFromDeviceOnImmediateCmdListThenDataisCorrectFromHost) {
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest<
      lzt::command_list_mode_t::immediate>(false);
}

LZT_TEST_F(
    zeCommandListAppendMemoryPrefetchTests,
    GivenMemoryPrefetchWhenWritingFromDeviceThenDataisCorrectFromHostWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest<
      lzt::command_list_mode_t::regular>(true);
}

LZT_TEST_F(
    zeCommandListAppendMemoryPrefetchTests,
    GivenMemoryPrefetchWhenWritingFromDeviceOnImmediateCmdListThenDataisCorrectFromHostWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest<
      lzt::command_list_mode_t::immediate>(true);
}

class zeCommandListAppendMemAdviseTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_advice_t, lzt::command_list_mode_t>> {
protected:
  void RunGivenMemAdviseWhenWritingFromDeviceTest(ze_memory_advice_t mem_advice,
                                                  lzt::command_list_mode_t mode,
                                                  bool is_shared_system);
};

void zeCommandListAppendMemAdviseTests::
    RunGivenMemAdviseWhenWritingFromDeviceTest(ze_memory_advice_t mem_advice,
                                               lzt::command_list_mode_t mode,
                                               bool is_shared_system) {
  const size_t size = 16;
  const uint8_t value = 0x55;
  void *memory =
      allocate_shared_memory_with_allocator_selector(size, is_shared_system);
  memset(memory, 0xaa, size);

  auto cmd_bundle = lzt::create_command_bundle(mode);

  lzt::append_memory_advise(cmd_bundle.list,
                            lzt::zeDevice::get_instance()->get_device(), memory,
                            size, mem_advice);

  lzt::append_memory_set(cmd_bundle.list, memory, &value, size);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(value, ((uint8_t *)memory)[i]);
  }

  free_memory_with_allocator_selector(memory, is_shared_system);
  lzt::destroy_command_bundle(cmd_bundle);
}

LZT_TEST_P(zeCommandListAppendMemAdviseTests,
           GivenMemAdviseWhenWritingFromDeviceThenDataIsCorrectFromHost) {
  ze_memory_advice_t mem_advice = std::get<0>(GetParam());
  lzt::command_list_mode_t mode = std::get<1>(GetParam());
  RunGivenMemAdviseWhenWritingFromDeviceTest(mem_advice, mode, false);
}

LZT_TEST_P(
    zeCommandListAppendMemAdviseTests,
    GivenMemAdviseWhenWritingFromDeviceThenDataIsCorrectFromHostWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  ze_memory_advice_t mem_advice = std::get<0>(GetParam());
  lzt::command_list_mode_t mode = std::get<1>(GetParam());
  RunGivenMemAdviseWhenWritingFromDeviceTest(mem_advice, mode, true);
}

INSTANTIATE_TEST_SUITE_P(
    MemAdviceFlags, zeCommandListAppendMemAdviseTests,
    ::testing::Combine(
        ::testing::Values(
            ZE_MEMORY_ADVICE_SET_READ_MOSTLY,
            ZE_MEMORY_ADVICE_CLEAR_READ_MOSTLY,
            ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION,
            ZE_MEMORY_ADVICE_CLEAR_PREFERRED_LOCATION,
            ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION,
            ZE_MEMORY_ADVICE_CLEAR_SYSTEM_MEMORY_PREFERRED_LOCATION,
            ZE_MEMORY_ADVICE_SET_NON_ATOMIC_MOSTLY,
            ZE_MEMORY_ADVICE_CLEAR_NON_ATOMIC_MOSTLY,
            ZE_MEMORY_ADVICE_BIAS_CACHED, ZE_MEMORY_ADVICE_BIAS_UNCACHED),
        ::testing::Values(lzt::command_list_mode_t::regular,
                          lzt::command_list_mode_t::immediate)));

class zeCommandListAppendMemoryCopyParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          ze_memory_type_t, ze_memory_type_t, lzt::command_list_mode_t>> {
public:
  void launchParamAppendMemCopy(ze_device_handle_t device, int *src_memory,
                                int *dst_memory, size_t size,
                                lzt::command_list_mode_t mode) {
    auto cmd_bundle = lzt::create_command_bundle(device, mode);
    const int8_t src_pattern = 1;
    const int8_t dst_pattern = 0;

    int *expected_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
    int *verify_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(expected_data),
                            static_cast<const void *>(&src_pattern),
                            sizeof(int), size * sizeof(int), nullptr);
    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(verify_data),
                            static_cast<const void *>(&dst_pattern),
                            sizeof(int), size * sizeof(int), nullptr);

    EXPECT_NE(expected_data, nullptr);
    EXPECT_NE(verify_data, nullptr);

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(src_memory),
                            static_cast<const void *>(&src_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<const void *>(&dst_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<void *>(src_memory),
                            size * sizeof(int));

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, verify_data,
                            static_cast<void *>(dst_memory),
                            size * sizeof(int));
    lzt::append_memory_copy(cmd_bundle.list, expected_data,
                            static_cast<void *>(src_memory),
                            size * sizeof(int));
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(expected_data, verify_data, size * sizeof(int)));

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

LZT_TEST_P(
    zeCommandListAppendMemoryCopyParameterizedTests,
    GivenParameterizedSourceAndDestinationMemAllocTypesWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  ze_memory_type_t src_memory_type = std::get<0>(GetParam());
  ze_memory_type_t dst_memory_type = std::get<1>(GetParam());
  lzt::command_list_mode_t mode = std::get<2>(GetParam());

  auto context = lzt::get_default_context();
  const size_t size = 16;

  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {

      int *src_memory = nullptr;
      int *dst_memory = nullptr;

      switch (src_memory_type) {
      case ZE_MEMORY_TYPE_DEVICE:
        src_memory = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
        break;
      case ZE_MEMORY_TYPE_HOST:
        src_memory = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
        break;
      case ZE_MEMORY_TYPE_SHARED:
        src_memory = static_cast<int *>(lzt::allocate_shared_memory(
            size * sizeof(int), 1, 0, 0, device, context));
        break;
      default:
        LOG_WARNING << "Unhandled memory type for parameterized append memory "
                       "copy test: "
                    << src_memory_type;
      }
      switch (dst_memory_type) {
      case ZE_MEMORY_TYPE_DEVICE:
        dst_memory = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
        break;
      case ZE_MEMORY_TYPE_HOST:
        dst_memory = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
        break;
      case ZE_MEMORY_TYPE_SHARED:
        dst_memory = static_cast<int *>(lzt::allocate_shared_memory(
            size * sizeof(int), 1, 0, 0, device, context));
        break;
      default:
        LOG_WARNING << "Unhandled memory type for parameterized append memory "
                       "copy test: "
                    << dst_memory_type;
      }

      EXPECT_NE(src_memory, nullptr);
      EXPECT_NE(dst_memory, nullptr);

      launchParamAppendMemCopy(device, src_memory, dst_memory, size, mode);

      if (dst_memory)
        lzt::free_memory(context, dst_memory);
      if (src_memory)
        lzt::free_memory(context, src_memory);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    ParamAppendMemCopy, zeCommandListAppendMemoryCopyParameterizedTests,
    ::testing::Combine(memory_types, memory_types,
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

enum MemoryType {
  MEMORY_TYPE_DEVICE,
  MEMORY_TYPE_HOST,
  MEMORY_TYPE_SHARED,
  MEMORY_TYPE_SHARED_SYSTEM
};

static std::string MemoryTypeString(MemoryType type) {
  switch (type) {
  case MEMORY_TYPE_DEVICE:
    return "MEMORY_TYPE_DEVICE";
  case MEMORY_TYPE_HOST:
    return "MEMORY_TYPE_HOST";
  case MEMORY_TYPE_SHARED:
    return "MEMORY_TYPE_SHARED";
  case MEMORY_TYPE_SHARED_SYSTEM:
    return "MEMORY_TYPE_SHARED_SYSTEM";
  default:
    return "UNKNOWN_MEMORY_TYPE";
  }
}

class AppendMemoryCopyWithSharedSystem
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<MemoryType, MemoryType, lzt::command_list_mode_t>> {
public:
  void launchAppendMemCopy(ze_device_handle_t device, int *src_memory,
                           int *dst_memory, size_t size,
                           lzt::command_list_mode_t mode) {
    auto cmd_bundle = lzt::create_command_bundle(device, mode);
    const int8_t src_pattern = 1;
    const int8_t dst_pattern = 0;

    int *expected_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));
    int *verify_data =
        static_cast<int *>(lzt::allocate_host_memory(size * sizeof(int)));

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(expected_data),
                            static_cast<const void *>(&src_pattern),
                            sizeof(int), size * sizeof(int), nullptr);
    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(verify_data),
                            static_cast<const void *>(&dst_pattern),
                            sizeof(int), size * sizeof(int), nullptr);

    EXPECT_NE(expected_data, nullptr);
    EXPECT_NE(verify_data, nullptr);

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(src_memory),
                            static_cast<const void *>(&src_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);

    lzt::append_memory_fill(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<const void *>(&dst_pattern),
                            sizeof(uint8_t), size * sizeof(int), nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<void *>(src_memory),
                            size * sizeof(int));

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, verify_data,
                            static_cast<void *>(dst_memory),
                            size * sizeof(int));
    lzt::append_memory_copy(cmd_bundle.list, expected_data,
                            static_cast<void *>(src_memory),
                            size * sizeof(int));
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(expected_data, verify_data, size * sizeof(int)));

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

LZT_TEST_P(
    AppendMemoryCopyWithSharedSystem,
    GivenParameterizedSourceAndDestinationMemAllocTypesWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {

  MemoryType src_memory_type = std::get<0>(GetParam());
  MemoryType dst_memory_type = std::get<1>(GetParam());
  lzt::command_list_mode_t mode = std::get<2>(GetParam());

  auto context = lzt::get_default_context();
  const size_t size = 16;

  std::vector<ze_device_handle_t> device_svm;
  uint32_t svm_count = 0;
  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      if (lzt::supports_shared_system_alloc(device)) {
        device_svm.push_back(device);
        svm_count++;
      }
    }
  }

  if (!svm_count) {
    GTEST_SKIP() << "No devices on any driver support shared system allocation";
  } else {
    for (auto device : device_svm) {
      int *src_memory = nullptr;
      int *dst_memory = nullptr;

      switch (src_memory_type) {
      case MEMORY_TYPE_DEVICE:
        src_memory = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
        break;
      case MEMORY_TYPE_HOST:
        src_memory = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
        break;
      case MEMORY_TYPE_SHARED:
        src_memory = static_cast<int *>(lzt::allocate_shared_memory(
            size * sizeof(int), 1, 0, 0, device, context));
        break;
      case MEMORY_TYPE_SHARED_SYSTEM:
        src_memory =
            static_cast<int *>(lzt::aligned_malloc(size * sizeof(int), 1));
        break;
      default:
        LOG_WARNING << "Unhandled memory type for parameterized append memory "
                       "copy test: "
                    << MemoryTypeString(src_memory_type);
      }
      switch (dst_memory_type) {
      case MEMORY_TYPE_DEVICE:
        dst_memory = static_cast<int *>(lzt::allocate_device_memory(
            size * sizeof(int), 1, 0u, device, context));
        break;
      case MEMORY_TYPE_HOST:
        dst_memory = static_cast<int *>(
            lzt::allocate_host_memory(size * sizeof(int), 1, context));
        break;
      case MEMORY_TYPE_SHARED:
        dst_memory = static_cast<int *>(lzt::allocate_shared_memory(
            size * sizeof(int), 1, 0, 0, device, context));
        break;
      case MEMORY_TYPE_SHARED_SYSTEM:
        dst_memory = static_cast<int *>(malloc(size * sizeof(int)));
        break;
      default:
        LOG_WARNING << "Unhandled memory type for parameterized append memory "
                       "copy test: "
                    << MemoryTypeString(dst_memory_type);
      }

      EXPECT_NE(src_memory, nullptr);
      EXPECT_NE(dst_memory, nullptr);

      launchAppendMemCopy(device, src_memory, dst_memory, size, mode);

      const bool is_shared_system_src =
          src_memory_type == MEMORY_TYPE_SHARED_SYSTEM;
      lzt::free_memory_with_allocator_selector(context, src_memory,
                                               is_shared_system_src);

      const bool is_shared_system_dst =
          dst_memory_type == MEMORY_TYPE_SHARED_SYSTEM;
      lzt::free_memory_with_allocator_selector(context, dst_memory,
                                               is_shared_system_dst);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    ParamAppendMemCopyWithSourceAsSharedSystem,
    AppendMemoryCopyWithSharedSystem,
    ::testing::Combine(::testing::Values(MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(MEMORY_TYPE_DEVICE, MEMORY_TYPE_HOST,
                                         MEMORY_TYPE_SHARED,
                                         MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

INSTANTIATE_TEST_SUITE_P(
    ParamAppendMemCopyWithDestinationAsSharedSystem,
    AppendMemoryCopyWithSharedSystem,
    ::testing::Combine(::testing::Values(MEMORY_TYPE_DEVICE, MEMORY_TYPE_HOST,
                                         MEMORY_TYPE_SHARED,
                                         MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

void *malloc_aligned(size_t alignment, size_t size) {
#ifdef __linux__
  return aligned_alloc(alignment, size);
#else // Windows
  return _aligned_malloc(size, alignment);
#endif
}

void free_aligned(void *ptr) {
#ifdef __linux__
  free(ptr);
#else // Windows
  _aligned_free(ptr);
#endif
}

class zeCommandListAppendMemoryCopySharedSystemUsmHostUserPtr
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, lzt::command_list_mode_t>> {
public:
  void launchHostUsrPtrAppendMemCopy(ze_device_handle_t device,
                                     char *src_memory, char *dst_memory,
                                     size_t size,
                                     lzt::command_list_mode_t mode) {

    auto cmd_bundle = lzt::create_command_bundle(device, mode);
    const int8_t src_pattern = 0x55;
    const int8_t dst_pattern = 0;

    char *expected_data = static_cast<char *>(lzt::allocate_host_memory(size));
    char *verify_data = static_cast<char *>(lzt::allocate_host_memory(size));

    memset(expected_data, src_pattern, size);
    memset(verify_data, dst_pattern, size);

    EXPECT_NE(expected_data, nullptr);
    EXPECT_NE(verify_data, nullptr);

    memset(src_memory, src_pattern, size);
    memset(dst_memory, dst_pattern, size);

    lzt::append_memory_copy(cmd_bundle.list, static_cast<void *>(dst_memory),
                            static_cast<void *>(src_memory), size);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_bundle.list, verify_data,
                            static_cast<void *>(dst_memory), size);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(expected_data, verify_data, size));

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

LZT_TEST_P(
    zeCommandListAppendMemoryCopySharedSystemUsmHostUserPtr,
    GivenSourceAndDestinationSharedSystemUsmWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  size_t size = std::get<0>(GetParam());
  lzt::command_list_mode_t mode = std::get<1>(GetParam());

  auto context = lzt::get_default_context();

  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {

      char *src_memory = nullptr;
      char *dst_memory = nullptr;

      src_memory = static_cast<char *>(
          lzt::allocate_device_memory_with_allocator_selector(size, true));
      dst_memory = static_cast<char *>(
          lzt::allocate_device_memory_with_allocator_selector(size, true));

      EXPECT_NE(src_memory, nullptr);
      EXPECT_NE(dst_memory, nullptr);

      launchHostUsrPtrAppendMemCopy(device, src_memory, dst_memory, size, mode);

      if (dst_memory) {
        lzt::free_memory_with_allocator_selector(dst_memory, true);
      }
      if (src_memory) {
        lzt::free_memory_with_allocator_selector(src_memory, true);
      }
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    ParamAppendMemCopyHostUserPtr,
    zeCommandListAppendMemoryCopySharedSystemUsmHostUserPtr,
    ::testing::Combine(::testing::Values(1, 16, 128, 4096, 65536),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

class zeSharedSystemMemoryCopyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_type_t, size_t, size_t>> {
protected:
  void SetUp() override {
    if (!lzt::supports_shared_system_alloc()) {
      GTEST_SKIP() << "Device does not support accessing shared system memory";
    }
  }

  void TearDown() override {}

  template <lzt::command_list_mode_t Mode>
  void run(const bool use_sys_dst, const bool close_and_reset_immediate) {
    const ze_memory_type_t memory_type = std::get<0>(GetParam());
    const size_t buf_sz = std::get<1>(GetParam());
    const size_t alignment = std::get<2>(GetParam());

    void *buf = nullptr;
    switch (memory_type) {
    case ZE_MEMORY_TYPE_HOST: {
      buf = lzt::allocate_host_memory(buf_sz, 1);
      break;
    }
    case ZE_MEMORY_TYPE_DEVICE: {
      buf = lzt::allocate_device_memory(buf_sz, 1);
      break;
    }
    case ZE_MEMORY_TYPE_SHARED: {
      buf = lzt::allocate_shared_memory(buf_sz, 1);
      break;
    }
    default: {
      FAIL() << "Unhandled memory type " << memory_type;
      break;
    }
    }
    EXPECT_NE(nullptr, buf);

    // Buffer size must be a multiple of the alignment
    const size_t buf_aligned_sz = (buf_sz % alignment == 0)
                                      ? buf_sz
                                      : (buf_sz / alignment + 1) * alignment;
    void *buf_sys = malloc_aligned(alignment, buf_aligned_sz);
    EXPECT_NE(nullptr, buf_sys);
    std::fill_n(static_cast<uint8_t *>(buf_sys), buf_aligned_sz, 0);

    void *buf_src = buf_sys;
    void *buf_dst = buf;
    if (use_sys_dst) {
      std::swap(buf_src, buf_dst);
    }

    auto cmd_bundle = lzt::create_command_bundle<Mode>();

    // In theory this doesn't affect immediate cmdlists, but it does in reality
    const bool close_and_reset = (Mode == lzt::command_list_mode_t::regular) ||
                                 ((Mode != lzt::command_list_mode_t::regular) &&
                                  close_and_reset_immediate);

    for (int i = 0; i < this->n_iters; i++) {
      lzt::append_memory_copy(cmd_bundle.list, buf_dst, buf_src, buf_sz);
      if (close_and_reset) {
        lzt::close_command_list(cmd_bundle.list);
      }
      lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
      if (close_and_reset) {
        lzt::reset_command_list(cmd_bundle.list);
      }
    }

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(buf);
    free_aligned(buf_sys);
  }

  const int n_iters = 1000;
};

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::regular>(true, false);
}

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::regular>(false, false);
}

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryOnImmediateCmdListWithCloseAndResetThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::immediate>(true, true);
}

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryOnImmediateCmdListWithCloseAndResetThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::immediate>(false, true);
}

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryOnImmediateCmdListWithoutCloseAndResetThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::immediate>(true, false);
}

LZT_TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryOnImmediateCmdListWithoutCloseAndResetThenSuccessIsReturned) {
  this->template run<lzt::command_list_mode_t::immediate>(false, false);
}

INSTANTIATE_TEST_SUITE_P(
    ParamSharedSystemMemCopy, zeSharedSystemMemoryCopyTests,
    ::testing::Combine(
        ::testing::Values(ZE_MEMORY_TYPE_HOST, ZE_MEMORY_TYPE_DEVICE,
                          ZE_MEMORY_TYPE_SHARED),
        ::testing::Values(1, 8, 1024, 4096, 65536, 1u << 21),
        ::testing::Values(1, 2, 4, 8, 16, 32, 64, 4096, 65536, 1u << 21)));

} // namespace
