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

#include <algorithm>
#include <cstdlib>
#include <array>
#include <string>

using namespace level_zero_tests;

namespace {

class zeCommandListAppendMemoryFillTests : public ::testing::Test {
protected:
  void RunMaxMemoryFillTest(bool is_immediate);
  void RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillTest(
      bool is_immediate);
  void RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(
      bool is_immediate);
  void RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
      bool is_immediate);
};

void zeCommandListAppendMemoryFillTests::
    RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillTest(
        bool is_immediate) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  const int pattern_size = 1;

  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillTest(true);
}

void zeCommandListAppendMemoryFillTests::RunMaxMemoryFillTest(
    bool is_immediate) {
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto cmd_q_group_properties = lzt::get_command_queue_group_properties(device);
  auto device_properties = lzt::get_device_properties(device);
  auto max_alloc_memsize = device_properties.maxMemAllocSize;

  for (int i = 0; i < cmd_q_group_properties.size(); i++) {
    auto bundle = lzt::create_command_bundle(lzt::get_default_context(), device,
                                             0, i, is_immediate);
    size_t size = cmd_q_group_properties[i].maxMemoryFillPatternSize;
    int pattern_size = cmd_q_group_properties[i].maxMemoryFillPatternSize;
    if (cmd_q_group_properties[i].maxMemoryFillPatternSize >
        max_alloc_memsize) {
      pattern_size = 1;
      size = 4096;
    }
    void *memory = allocate_device_memory(size);
    const uint8_t pattern = 0x00;

    lzt::append_memory_fill(bundle.list, memory, &pattern, pattern_size, size,
                            nullptr);

    close_command_list(bundle.list);
    execute_and_sync_command_bundle(bundle, UINT64_MAX);
    reset_command_list(bundle.list);
    free_memory(memory);
    lzt::destroy_command_bundle(bundle);
  }
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunMaxMemoryFillTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenImmediateCommandListAndMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned) {
  RunMaxMemoryFillTest(true);
}

void zeCommandListAppendMemoryFillTests::
    RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(
        bool is_immediate) {
  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  ep.create_event(hEvent);
  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          hEvent);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventTest(true);
}

void zeCommandListAppendMemoryFillTests::
    RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
        bool is_immediate) {
  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                          nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
      false);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventTest(
      true);
}

class zeCommandListAppendMemoryFillVerificationTests : public ::testing::Test {
protected:
  void RunGivenHostMemoryWhenExecutingAMemoryFillTest(bool is_immediate);
  void RunGivenSharedMemoryWhenExecutingAMemoryFillTest(bool is_immediate);
  void RunGivenDeviceMemoryWhenExecutingAMemoryFillTest(bool is_immediate);
};

void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenHostMemoryWhenExecutingAMemoryFillTest(bool is_immediate) {
  size_t size = 16;
  auto memory = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenHostMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenHostMemoryWhenExecutingAMemoryFillTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenHostMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenHostMemoryWhenExecutingAMemoryFillTest(true);
}

void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenSharedMemoryWhenExecutingAMemoryFillTest(bool is_immediate) {
  size_t size = 16;
  auto memory = allocate_shared_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
}

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenSharedMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenSharedMemoryWhenExecutingAMemoryFillTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenSharedMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenSharedMemoryWhenExecutingAMemoryFillTest(true);
}

void zeCommandListAppendMemoryFillVerificationTests::
    RunGivenDeviceMemoryWhenExecutingAMemoryFillTest(bool is_immediate) {
  size_t size = 16;
  auto memory = allocate_device_memory(size);
  auto local_mem = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  append_memory_fill(cmd_bundle.list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(cmd_bundle.list));
  append_memory_copy(cmd_bundle.list, local_mem, memory, size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(local_mem)[i], pattern)
        << "Memory Fill did not match.";
  }

  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(memory);
  free_memory(local_mem);
}

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenDeviceMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {
  RunGivenDeviceMemoryWhenExecutingAMemoryFillTest(false);
}

TEST_F(
    zeCommandListAppendMemoryFillVerificationTests,
    GivenDeviceMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly) {
  RunGivenDeviceMemoryWhenExecutingAMemoryFillTest(true);
}

class zeCommandListAppendMemoryFillSubDeviceVerificationTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<ze_memory_type_t, bool>> {
};
TEST_P(
    zeCommandListAppendMemoryFillSubDeviceVerificationTests,
    GivenMemoryAllocationWhenExecutingAMemoryFillWithSubDeviceThenMemoryIsSetCorrectly) {

  auto memory_type = std::get<0>(GetParam());
  bool is_immediate = std::get<1>(GetParam());
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
      auto cmd_bundle = lzt::create_command_bundle(subdevice, is_immediate);

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
      lzt::close_command_list(cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

      for (int i = 0; i < size; i++) {
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
                       ::testing::Bool()));

class zeCommandListAppendMemoryFillPatternVerificationTests
    : public zeCommandListAppendMemoryFillVerificationTests,
      public ::testing::WithParamInterface<std::tuple<size_t, bool>> {};

TEST_P(zeCommandListAppendMemoryFillPatternVerificationTests,
       GivenPatternSizeWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {

  const int pattern_size = std::get<0>(GetParam());
  const bool is_immediate = std::get<1>(GetParam());
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t total_size = (pattern_size * 10) + 5;
  std::unique_ptr<uint8_t> pattern(new uint8_t[pattern_size]);
  auto target_memory = allocate_host_memory(total_size);

  for (uint32_t i = 0; i < pattern_size; i++) {
    pattern.get()[i] = i;
  }

  append_memory_fill(cmd_bundle.list, target_memory, pattern.get(),
                     pattern_size, total_size, nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < total_size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(target_memory)[i], i % pattern_size)
        << "Memory Fill did not match.";
  }
  lzt::destroy_command_bundle(cmd_bundle);
  free_memory(target_memory);
}

INSTANTIATE_TEST_SUITE_P(VaryPatternSize,
                         zeCommandListAppendMemoryFillPatternVerificationTests,
                         ::testing::Combine(::testing::Values(1, 2, 4, 8, 16,
                                                              32, 64, 128),
                                            ::testing::Bool()));

class zeCommandListCommandQueueTests : public ::testing::Test {};

class zeCommandListAppendMemoryCopyWithDataVerificationTests
    : public zeCommandListCommandQueueTests {
protected:
  void RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(
      bool is_immediate);
  void RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(
      bool is_immediate);
};

void zeCommandListAppendMemoryCopyWithDataVerificationTests::
    RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(
        bool is_immediate) {
  const size_t size = 4 * 1024;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(size_in_bytes(host_memory1));
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  append_memory_copy(cmd_bundle.list, device_memory, host_memory1.data(),
                     size_in_bytes(host_memory1), nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  append_memory_copy(cmd_bundle.list, host_memory2.data(), device_memory,
                     size_in_bytes(host_memory2), nullptr);
  append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  free_memory(device_memory);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(false);
}

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(true);
}

void zeCommandListAppendMemoryCopyWithDataVerificationTests::
    RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(
        bool is_immediate) {
  const size_t size = 1024;
  // number of transfers to device memory
  const size_t num_dev_mem_copy = 8;
  // byte address alignment
  const size_t addr_alignment = 1;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(
      num_dev_mem_copy * (size_in_bytes(host_memory1) + addr_alignment));
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

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
  close_command_list(cmd_bundle.list);
  execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);

  free_memory(device_memory);
  lzt::destroy_command_bundle(cmd_bundle);
}

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(false);
}

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyTest(true);
}

class zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests
    : public zeCommandListCommandQueueTests {
protected:
  void
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest(
      bool is_immediate);
  void
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest(
      bool is_immediate);
};

void zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests::
    RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest(
        bool is_immediate) {
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
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

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
  close_command_list(cmd_bundle.list);
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

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest(
      false);
}

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyTest(
      true);
}

void zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests::
    RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest(
        bool is_immediate) {
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
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

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
  close_command_list(cmd_bundle.list);
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

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest(
      false);
}

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect) {
  RunGivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsTest(
      true);
}

class zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          size_t, size_t, size_t, ze_memory_type_t, ze_memory_type_t, bool>> {
protected:
  zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests() {
    rows = std::get<0>(GetParam());
    columns = std::get<1>(GetParam());
    slices = std::get<2>(GetParam());

    memory_size = rows * columns * slices;

    ze_memory_type_t source_type = std::get<3>(GetParam());
    ze_memory_type_t destination_type = std::get<4>(GetParam());

    is_immediate = std::get<5>(GetParam());
    cmd_bundle = lzt::create_command_bundle(is_immediate);

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

    std::array<size_t, 3> widths = {1, columns / 2, columns};
    std::array<size_t, 3> heights = {1, rows / 2, rows};
    std::array<size_t, 3> depths = {1, slices / 2, slices};

    for (int region = 0; region < 3; region++) {
      // Define region to be copied from/to
      auto width = widths[region];
      auto height = heights[region];
      auto depth = depths[region];

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
                                &dest_region, columns, columns * rows,
                                source_memory, &src_region, columns,
                                columns * rows, nullptr);
      append_barrier(cmd_bundle.list);
      append_memory_copy(cmd_bundle.list, verification_memory,
                         destination_memory, memory_size);
      close_command_list(cmd_bundle.list);
      execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
      reset_command_list(cmd_bundle.list);

      for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
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

  lzt::zeCommandBundle cmd_bundle;

  void *source_memory, *temp_src;
  void *destination_memory, *temp_dest;

  size_t rows;
  size_t columns;
  size_t slices;
  bool is_immediate;
  size_t memory_size;
};

TEST_P(
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
                       ::testing::Bool()));
class
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, size_t, size_t, bool>> {
protected:
  void RunMemoryCopyRegionTest() {

    is_immediate = std::get<3>(GetParam());
    cmd_bundle = lzt::create_command_bundle(is_immediate);

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

    std::array<size_t, 3> widths = {1, columns / 2, columns};
    std::array<size_t, 3> heights = {1, rows / 2, rows};
    std::array<size_t, 3> depths = {1, slices / 2, slices};

    for (int region = 0; region < 3; region++) {
      // Define region to be copied from/to
      auto width = widths[region];
      auto height = heights[region];
      auto depth = depths[region];

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
                                &dest_region, columns, columns * rows,
                                source_memory, &src_region, columns,
                                columns * rows, nullptr);
      append_barrier(cmd_bundle.list);
      append_memory_copy(cmd_bundle.list, verification_memory,
                         destination_memory, memory_size);
      close_command_list(cmd_bundle.list);
      execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);
      reset_command_list(cmd_bundle.list);

      for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
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

  lzt::zeCommandBundle cmd_bundle;

  void *source_memory, *temp_src;
  void *destination_memory, *temp_dest;

  size_t rows;
  size_t columns;
  size_t slices;
  bool is_immediate;
  size_t memory_size;
};

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToHostMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_host_memory(memory_size);
  RunMemoryCopyRegionTest();
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_shared_memory(memory_size);
  RunMemoryCopyRegionTest();
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToDeviceMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::allocate_device_memory(memory_size);
  RunMemoryCopyRegionTest();
  lzt::aligned_free(source_memory);
  lzt::free_memory(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidSharedSystemMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::aligned_malloc(memory_size, 1);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest();
  lzt::aligned_free(source_memory);
  lzt::aligned_free(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidHostMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_host_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest();
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidSharedMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_shared_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest();
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    GivenValidDeviceMemoryWhenAppendingMemoryCopyWithRegionToSharedSystemMemoryThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  rows = std::get<0>(GetParam());
  columns = std::get<1>(GetParam());
  slices = std::get<2>(GetParam());
  memory_size = rows * columns * slices;
  source_memory = lzt::allocate_device_memory(memory_size);
  destination_memory = lzt::aligned_malloc(memory_size, 1);
  RunMemoryCopyRegionTest();
  lzt::free_memory(source_memory);
  lzt::aligned_free(destination_memory);
}

INSTANTIATE_TEST_SUITE_P(
    MemoryCopies,
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTestsWithSharedSystem,
    ::testing::Combine(::testing::Values(8, 64),    // Rows
                       ::testing::Values(8, 64),    // Cols
                       ::testing::Values(1, 8, 64), // Slices
                       ::testing::Bool()));

class zeCommandListAppendMemoryCopyTests : public ::testing::Test {
protected:
  void
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(bool is_immediate,
                                                       bool is_shared_system);
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(
      bool is_immediate, bool is_shared_system);
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(
      bool is_immediate, bool is_shared_system);
  void RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
      bool is_immediate, bool is_shared_system);
};

void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(
        bool is_immediate, bool is_shared_system) {
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);

  append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                     size_in_bytes(host_memory), nullptr);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(false, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(true, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(false, true);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyTest(true, true);
}

void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(
        bool is_immediate, bool is_shared_system) {
  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                     size_in_bytes(host_memory), hEvent);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(false, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(true, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(false, true);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithHEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventTest(true, true);
}

void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(
        bool is_immediate, bool is_shared_system) {
  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  append_memory_copy(cmd_bundle.list, memory, host_memory.data(),
                     size_in_bytes(host_memory), nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(false,
                                                                    false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(true,
                                                                    false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(false,
                                                                    true);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyToSharedSystemMemoryWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventTest(true, true);
}

void zeCommandListAppendMemoryCopyTests::
    RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
        bool is_immediate, bool is_shared_system) {

  lzt::zeEventPool ep;
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = lzt::allocate_device_memory_with_allocator_selector(
      size_in_bytes(host_memory), is_shared_system);
  ze_event_handle_t hEvent = nullptr;
  ze_copy_region_t dstRegion = {};
  ze_copy_region_t srcRegion = {};

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::signal_event_from_host(hEvent);
  append_memory_copy_region(cmd_bundle.list, memory, &dstRegion, 0, 0,
                            host_memory.data(), &srcRegion, 0, 0, nullptr, 1,
                            &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  }
  ep.destroy_event(hEvent);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
      false, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(
      true, false);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionToSharedSystemMemoryWithWaitEventThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(false,
                                                                          true);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionToSharedSystemMemoryWithWaitEventOnImmediateCmdListThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventTest(true,
                                                                          true);
}

class zeCommandListAppendMemoryPrefetchTests : public ::testing::Test {
protected:
  void RunGivenMemoryPrefetchWhenWritingFromDeviceTest(bool is_immediate) {
    const size_t size = 16;
    const uint8_t value = 0x55;
    void *memory = allocate_shared_memory(size);
    memset(memory, 0xaa, size);
    auto cmd_bundle = lzt::create_command_bundle(is_immediate);

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendMemoryPrefetch(cmd_bundle.list, memory, size));
    lzt::append_memory_set(cmd_bundle.list, memory, &value, size);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(value, ((uint8_t *)memory)[i]);
    }

    free_memory(memory);
    lzt::destroy_command_bundle(cmd_bundle);
  }
};

TEST_F(zeCommandListAppendMemoryPrefetchTests,
       GivenMemoryPrefetchWhenWritingFromDeviceThenDataisCorrectFromHost) {
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest(false);
}

TEST_F(
    zeCommandListAppendMemoryPrefetchTests,
    GivenMemoryPrefetchWhenWritingFromDeviceOnImmediateCmdListThenDataisCorrectFromHost) {
  RunGivenMemoryPrefetchWhenWritingFromDeviceTest(true);
}

class zeCommandListAppendMemAdviseTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_advice_t, bool>> {};

TEST_P(zeCommandListAppendMemAdviseTests,
       GivenMemAdviseWhenWritingFromDeviceThenDataIsCorrectFromHost) {
  const size_t size = 16;
  const uint8_t value = 0x55;
  void *memory = allocate_shared_memory(size);
  memset(memory, 0xaa, size);
  ze_memory_advice_t mem_advice = std::get<0>(GetParam());
  bool is_immediate = std::get<1>(GetParam());
  auto cmd_bundle = lzt::create_command_bundle(is_immediate);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemAdvise(cmd_bundle.list,
                                         zeDevice::get_instance()->get_device(),
                                         memory, size, mem_advice));

  lzt::append_memory_set(cmd_bundle.list, memory, &value, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(value, ((uint8_t *)memory)[i]);
  }

  free_memory(memory);
  lzt::destroy_command_bundle(cmd_bundle);
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
        ::testing::Bool()));

class zeCommandListAppendMemoryCopyParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_type_t, ze_memory_type_t, bool>> {
public:
  void launchParamAppendMemCopy(ze_device_handle_t device, int *src_memory,
                                int *dst_memory, size_t size,
                                bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);
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
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(expected_data, verify_data, size * sizeof(int)));

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

TEST_P(
    zeCommandListAppendMemoryCopyParameterizedTests,
    GivenParameterizedSourceAndDestinationMemAllocTypesWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  ze_memory_type_t src_memory_type = std::get<0>(GetParam());
  ze_memory_type_t dst_memory_type = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());

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

      launchParamAppendMemCopy(device, src_memory, dst_memory, size,
                               is_immediate);

      if (dst_memory)
        lzt::free_memory(context, dst_memory);
      if (src_memory)
        lzt::free_memory(context, src_memory);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(ParamAppendMemCopy,
                         zeCommandListAppendMemoryCopyParameterizedTests,
                         ::testing::Combine(memory_types, memory_types,
                                            ::testing::Bool()));

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

class zeCommandListAppendMemoryCopyParameterizedTestsWithSharedSystem
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<MemoryType, MemoryType, bool>> {
public:
  void launchAppendMemCopy(ze_device_handle_t device, int *src_memory,
                           int *dst_memory, size_t size, bool is_immediate) {
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);
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
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(0, memcmp(expected_data, verify_data, size * sizeof(int)));

    lzt::destroy_command_bundle(cmd_bundle);
    lzt::free_memory(expected_data);
    lzt::free_memory(verify_data);
  }
};

TEST_P(
    zeCommandListAppendMemoryCopyParameterizedTestsWithSharedSystem,
    GivenParameterizedSourceAndDestinationMemAllocTypesWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrectWithSharedSystemAllocator) {

  MemoryType src_memory_type = std::get<0>(GetParam());
  MemoryType dst_memory_type = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());

  auto context = lzt::get_default_context();
  const size_t size = 16;

  for (auto driver : lzt::get_all_driver_handles()) {
    for (auto device : lzt::get_devices(driver)) {
      if (!lzt::supports_shared_system_alloc(device)) {
        LOG_WARNING << "Device does not support shared system allocation";
        continue;
      }
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

      launchAppendMemCopy(device, src_memory, dst_memory, size, is_immediate);

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
    zeCommandListAppendMemoryCopyParameterizedTestsWithSharedSystem,
    ::testing::Combine(::testing::Values(MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(MEMORY_TYPE_DEVICE, MEMORY_TYPE_HOST,
                                         MEMORY_TYPE_SHARED,
                                         MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Bool()));

INSTANTIATE_TEST_SUITE_P(
    ParamAppendMemCopyWithDestinationAsSharedSystem,
    zeCommandListAppendMemoryCopyParameterizedTestsWithSharedSystem,
    ::testing::Combine(::testing::Values(MEMORY_TYPE_DEVICE, MEMORY_TYPE_HOST,
                                         MEMORY_TYPE_SHARED,
                                         MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Values(MEMORY_TYPE_SHARED_SYSTEM),
                       ::testing::Bool()));

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

  void run(const bool use_sys_dst, const bool is_immediate,
           const bool close_and_reset_immediate) {
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

    auto cmd_bundle = lzt::create_command_bundle(is_immediate);

    // In theory this doesn't affect immediate cmdlists, but it does in reality
    const bool close_and_reset =
        !is_immediate || (is_immediate && close_and_reset_immediate);

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

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryThenSuccessIsReturned) {
  this->run(true, false, false);
}

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryThenSuccessIsReturned) {
  this->run(false, false, false);
}

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryOnImmediateCmdListWithCloseAndResetThenSuccessIsReturned) {
  this->run(true, true, true);
}

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryOnImmediateCmdListWithCloseAndResetThenSuccessIsReturned) {
  this->run(false, true, true);
}

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingToSharedSystemMemoryOnImmediateCmdListWithoutCloseAndResetThenSuccessIsReturned) {
  this->run(true, true, false);
}

TEST_P(
    zeSharedSystemMemoryCopyTests,
    GivenSizeAndAlignmentWhenCopyingFromSharedSystemMemoryOnImmediateCmdListWithoutCloseAndResetThenSuccessIsReturned) {
  this->run(false, true, false);
}

INSTANTIATE_TEST_SUITE_P(
    ParamSharedSystemMemCopy, zeSharedSystemMemoryCopyTests,
    ::testing::Combine(
        ::testing::Values(ZE_MEMORY_TYPE_HOST, ZE_MEMORY_TYPE_DEVICE,
                          ZE_MEMORY_TYPE_SHARED),
        ::testing::Values(1, 8, 1024, 4096, 65536, 1u << 21),
        ::testing::Values(1, 2, 4, 8, 16, 32, 64, 4096, 65536, 1u << 21)));

} // namespace
