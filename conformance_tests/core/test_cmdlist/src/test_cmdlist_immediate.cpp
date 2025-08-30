/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "random/random.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <chrono>

namespace {

using lzt::to_nanoseconds;

using lzt::to_u8;
using lzt::to_int;
using lzt::to_u32;
using lzt::to_u64;
using lzt::to_f32;
using lzt::to_f64;

class zeImmediateCommandListInOrderExecutionTests
    : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    ep.InitEventPool(10);

    cmdlist_immediate_sync_mode = lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    cmdlist_immediate_async_mode = lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
    cmdlist_immediate_default_mode = lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  }

  void TearDown() override {
    lzt::destroy_command_list(cmdlist_immediate_sync_mode);
    lzt::destroy_command_list(cmdlist_immediate_async_mode);
    lzt::destroy_command_list(cmdlist_immediate_default_mode);
  }
  ze_command_list_handle_t cmdlist_immediate_sync_mode = nullptr;
  ze_command_list_handle_t cmdlist_immediate_async_mode = nullptr;
  ze_command_list_handle_t cmdlist_immediate_default_mode = nullptr;
};

class zeImmediateCommandListEventCounterTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    ze_event_pool_desc_t eventPoolDesc = {};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 10;

    ze_event_pool_counter_based_exp_desc_t counterBasedExtension = {
        ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedExtension.flags =
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_IMMEDIATE;
    eventPoolDesc.pNext = &counterBasedExtension;
    ep.InitEventPool(eventPoolDesc);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, 0);

    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
  }

  void TearDown() override {
    ep.destroy_event(event0);
    for (auto cl : cmdlist) {
      lzt::destroy_command_list(cl);
    }
  }
  std::vector<ze_command_list_handle_t> cmdlist;
  ze_event_handle_t event0 = nullptr;
};

class zeImmediateCommandListExecutionTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_mode_t, bool>> {

protected:
  void RunGivenImmediateCommandListWhenAppendLaunchKernelInstructionTest(
      bool is_shared_system);

  void SetUp() override {
    mode = std::get<0>(GetParam());
    useEventsToSynchronize = std::get<1>(GetParam());

    if (mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      timeout = 0;
    } else {
      timeout = UINT64_MAX - 1;
    }

    ep.InitEventPool(10);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);

    cmdlist_immediate = lzt::create_immediate_command_list(
        lzt::zeDevice::get_instance()->get_device(), 0, mode,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  }

  void TearDown() override {
    ep.destroy_event(event0);
    lzt::destroy_command_list(cmdlist_immediate);
  }
  ze_event_handle_t event0 = nullptr;
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  ze_command_queue_mode_t mode;
  bool useEventsToSynchronize = true;
  uint64_t timeout = UINT64_MAX - 1;
};

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendingMemorySetInstructionThenVerifyImmediateExecution) {
  const size_t size = 16;
  const uint8_t one = 1;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));

  // setting event on following instruction should flush memory
  lzt::append_memory_set(cmdlist_immediate, buffer, &one, size * sizeof(int),
                         event0);
  // command queue execution should be immediate, and so no timeout required for
  // synchronize
  if (useEventsToSynchronize) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
  }

  for (size_t i = 0; i < size * sizeof(int); i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[i], 0x1);
  }

  lzt::free_memory(buffer);
}

static void
RunAppendMemorySetInstructionTest(ze_command_list_handle_t cmdlist_immediate,
                                  ze_command_queue_mode_t mode) {
  const size_t size = 16;
  const uint8_t one = 1;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  lzt::append_memory_set(cmdlist_immediate, buffer, &one, size * sizeof(int),
                         nullptr);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    const uint64_t timeout = UINT64_MAX - 1;
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  }

  for (size_t i = 0; i < size * sizeof(int); i++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[i], 0x1);
  }

  lzt::free_memory(buffer);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendingMemorySetInstructionThenVerifyImmediateExecution) {
  RunAppendMemorySetInstructionTest(cmdlist_immediate_default_mode,
                                    ZE_COMMAND_QUEUE_MODE_DEFAULT);
  RunAppendMemorySetInstructionTest(cmdlist_immediate_sync_mode,
                                    ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS);
  RunAppendMemorySetInstructionTest(cmdlist_immediate_async_mode,
                                    ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS);
}

void zeImmediateCommandListExecutionTests::
    RunGivenImmediateCommandListWhenAppendLaunchKernelInstructionTest(
        bool is_shared_system) {
  const size_t size = 16;
  const int addval = 10;
  const int addval2 = 15;

  void *buffer = lzt::allocate_shared_memory_with_allocator_selector(
      size * sizeof(int), is_shared_system);

  memset(buffer, 0x0, size * sizeof(int));

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int *p_dev = static_cast<int *>(buffer);
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = to_u32(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;
  // setting event on following instruction should flush memory
  lzt::append_launch_function(cmdlist_immediate, kernel, &tg, event0, 0,
                              nullptr);
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event0));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<int *>(buffer)[i], addval);
  }
  lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);
  // setting event on following instruction should flush memory
  lzt::append_launch_function(cmdlist_immediate, kernel, &tg, event0, 0,
                              nullptr);
  if (useEventsToSynchronize) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
  }
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<int *>(buffer)[i], (addval + addval2));
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(buffer, is_shared_system);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendLaunchKernelInstructionThenVerifyImmediateExecution) {
  RunGivenImmediateCommandListWhenAppendLaunchKernelInstructionTest(false);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendLaunchKernelInstructionThenVerifyImmediateExecutionWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenImmediateCommandListWhenAppendLaunchKernelInstructionTest(true);
}

static void RunAppendLaunchKernel(ze_command_list_handle_t cmdlist_immediate,
                                  ze_command_queue_mode_t mode,
                                  bool is_shared_system) {
  const size_t size = 16;
  const int addval = 10;
  int addval2 = 15;
  const uint64_t timeout = UINT64_MAX - 1;

  void *buffer = lzt::allocate_shared_memory_with_allocator_selector(
      size * sizeof(int), is_shared_system);

  memset(buffer, 0x0, size * sizeof(int));

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int *p_dev = static_cast<int *>(buffer);
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = to_u32(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  lzt::append_launch_function(cmdlist_immediate, kernel, &tg, nullptr, 0,
                              nullptr);

  int totalVal = 0;
  for (int i = 0; i < 100; i++) {
    addval2 = addval2 + i;
    totalVal += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

    lzt::append_launch_function(cmdlist_immediate, kernel, &tg, nullptr, 0,
                                nullptr);
  }

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  }
  for (size_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<int *>(buffer)[i], (addval + totalVal));
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(buffer, is_shared_system);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendLaunchKernelInstructionThenVerifyImmediateExecution) {
  RunAppendLaunchKernel(cmdlist_immediate_default_mode,
                        ZE_COMMAND_QUEUE_MODE_DEFAULT, false);
  RunAppendLaunchKernel(cmdlist_immediate_sync_mode,
                        ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, false);
  RunAppendLaunchKernel(cmdlist_immediate_async_mode,
                        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, false);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendLaunchKernelInstructionThenVerifyImmediateExecutionWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendLaunchKernel(cmdlist_immediate_default_mode,
                        ZE_COMMAND_QUEUE_MODE_DEFAULT, true);
  RunAppendLaunchKernel(cmdlist_immediate_sync_mode,
                        ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, true);
  RunAppendLaunchKernel(cmdlist_immediate_async_mode,
                        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, true);
}

static void
RunAppendLaunchKernelEvent(std::vector<ze_command_list_handle_t> cmdlist,
                           ze_event_handle_t event, size_t num_cmdlist,
                           bool is_shared_system) {
  const size_t size = 16;
  const int addval = 10;
  const int num_iterations = 100;
  int addval2 = 0;
  const uint64_t timeout = UINT64_MAX - 1;

  void *buffer = lzt::allocate_shared_memory_with_allocator_selector(
      num_cmdlist * size * sizeof(int), is_shared_system);

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int totalVal[10];

  memset(buffer, 0x0, num_cmdlist * size * sizeof(int));

  for (size_t n = 0; n < num_cmdlist; n++) {
    int *p_dev = static_cast<int *>(buffer);
    p_dev += (n * size);
    lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    ze_group_count_t tg;
    tg.groupCountX = to_u32(size);
    tg.groupCountY = 1;
    tg.groupCountZ = 1;

    lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);

    totalVal[n] = 0;

    for (size_t i = 0; i < (num_iterations - 1); i++) {
      addval2 = lzt::generate_value<int>() & 0xFFFF;
      totalVal[n] += addval2;
      lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

      lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);
    }
    addval2 = lzt::generate_value<int>() & 0xFFFF;

    totalVal[n] += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);
    if (n == 0) {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 0, nullptr);
    } else {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 0, &event);
    }
  }
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, timeout));

  for (size_t n = 0; n < num_cmdlist; n++) {
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[(n * size) + i],
                (addval + totalVal[n]));
    }
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(buffer, is_shared_system);
}

LZT_TEST_F(
    zeImmediateCommandListEventCounterTests,
    GivenInOrderImmediateCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution) {
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }
  for (size_t i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Running " << i << " command list(s)";
    RunAppendLaunchKernelEvent(cmdlist, event0, i, false);
  }
}

LZT_TEST_F(
    zeImmediateCommandListEventCounterTests,
    GivenInOrderImmediateCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();

  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }
  for (size_t i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Running " << i << " command list(s)";
    RunAppendLaunchKernelEvent(cmdlist, event0, i, true);
  }
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendMemoryCopyThenVerifyCopyIsCorrect) {
  const size_t size = 4096;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory =
      lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  // This should execute immediately
  lzt::append_memory_copy(cmdlist_immediate, device_memory, host_memory1.data(),
                          lzt::size_in_bytes(host_memory1), event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeEventHostSynchronize(event0, timeout);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
    zeEventHostReset(event0);
  }
  lzt::append_memory_copy(cmdlist_immediate, host_memory2.data(), device_memory,
                          lzt::size_in_bytes(host_memory2), event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeEventHostSynchronize(event0, timeout);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
  }
  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  lzt::free_memory(device_memory);
}

static void RunAppendMemoryCopy(ze_command_list_handle_t cmdlist_immediate,
                                ze_command_queue_mode_t mode) {
  const uint64_t timeout = UINT64_MAX - 1;
  const size_t size = 4096;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory =
      lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  // This should execute immediately
  lzt::append_memory_copy(cmdlist_immediate, device_memory, host_memory1.data(),
                          lzt::size_in_bytes(host_memory1), nullptr);

  lzt::append_memory_copy(cmdlist_immediate, host_memory2.data(), device_memory,
                          lzt::size_in_bytes(host_memory2), nullptr);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  }
  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  lzt::free_memory(device_memory);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendMemoryCopyThenVerifyCopyIsCorrect) {
  RunAppendMemoryCopy(cmdlist_immediate_default_mode,
                      ZE_COMMAND_QUEUE_MODE_DEFAULT);
  RunAppendMemoryCopy(cmdlist_immediate_sync_mode,
                      ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS);
  RunAppendMemoryCopy(cmdlist_immediate_async_mode,
                      ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenMultipleImmediateCommandListWhenAppendCommandsToMultipleCommandListThenVerifyResultIsCorrect) {

  auto device_handle = lzt::zeDevice::get_instance()->get_device();

  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device_handle);

  uint32_t num_cmdq = 0;
  for (auto properties : command_queue_group_properties) {
    num_cmdq += properties.numQueues;
  }

  auto context_handle = lzt::get_default_context();
  ze_event_pool_desc_t event_pool_desc = {
      ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE, num_cmdq};

  auto event_pool_handle =
      lzt::create_event_pool(context_handle, event_pool_desc);

  std::vector<ze_command_list_handle_t> mulcmdlist_immediate(num_cmdq, nullptr);
  std::vector<void *> buffer(num_cmdq, nullptr);
  std::vector<uint8_t> val(num_cmdq, 0);
  const size_t size = 16;

  std::vector<ze_event_handle_t> host_to_dev_event(num_cmdq, nullptr);
  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.pNext = nullptr;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

  for (uint32_t i = 0; i < num_cmdq; i++) {
    mulcmdlist_immediate[i] = lzt::create_immediate_command_list(
        context_handle, device_handle, 0, mode,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);

    buffer[i] = lzt::allocate_shared_memory(size);
    val[i] = to_u8(i + 1);
    event_desc.index = i;
    host_to_dev_event[i] = lzt::create_event(event_pool_handle, event_desc);

    // This should execute immediately
    lzt::append_memory_set(mulcmdlist_immediate[i], buffer[i], &val[i], size,
                           host_to_dev_event[i]);
    if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      if (useEventsToSynchronize) {
        zeEventHostSynchronize(host_to_dev_event[i], timeout);
      } else {
        EXPECT_ZE_RESULT_SUCCESS(
            zeCommandListHostSynchronize(mulcmdlist_immediate[i], timeout));
        EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(host_to_dev_event[i]));
      }
    }
  }

  // validate event status and validate buffer copied/set
  for (uint32_t i = 0; i < num_cmdq; i++) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(host_to_dev_event[i]));
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], val[i]);
    }
  }

  for (uint32_t i = 0; i < num_cmdq; i++) {
    lzt::destroy_event(host_to_dev_event[i]);
    lzt::destroy_command_list(mulcmdlist_immediate[i]);
    lzt::free_memory(buffer[i]);
  }
  lzt::destroy_event_pool(event_pool_handle);
}

static void RunMultipleAppendMemoryCopy(ze_command_queue_mode_t mode) {
  const uint64_t timeout = UINT64_MAX - 1;
  auto device_handle = lzt::zeDevice::get_instance()->get_device();

  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device_handle);

  uint32_t num_cmdq = 0;
  for (auto properties : command_queue_group_properties) {
    num_cmdq += properties.numQueues;
  }

  auto context_handle = lzt::get_default_context();

  std::vector<ze_command_list_handle_t> mulcmdlist_immediate(num_cmdq, nullptr);
  std::vector<void *> buffer(num_cmdq, nullptr);
  std::vector<uint8_t> val(num_cmdq, 0);
  const size_t size = 16;

  for (uint32_t i = 0; i < num_cmdq; i++) {
    mulcmdlist_immediate[i] = lzt::create_immediate_command_list(
        context_handle, device_handle, ZE_COMMAND_QUEUE_FLAG_IN_ORDER, mode,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);

    buffer[i] = lzt::allocate_shared_memory(size);
    val[i] = to_u8(i + 1);

    // This should execute immediately
    lzt::append_memory_set(mulcmdlist_immediate[i], buffer[i], &val[i], size,
                           nullptr);
  }

  for (uint32_t i = 0; i < num_cmdq; i++) {
    if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(mulcmdlist_immediate[i], timeout));
    }
  }

  // validate buffer copied/set
  for (uint32_t i = 0; i < num_cmdq; i++) {
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], val[i]);
    }
  }

  for (uint32_t i = 0; i < num_cmdq; i++) {
    lzt::destroy_command_list(mulcmdlist_immediate[i]);
    lzt::free_memory(buffer[i]);
  }
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenMultipleInOrderImmediateCommandListWhenAppendCommandsToMultipleCommandListThenVerifyResultIsCorrect) {
  RunMultipleAppendMemoryCopy(ZE_COMMAND_QUEUE_MODE_DEFAULT);
  RunMultipleAppendMemoryCopy(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS);
  RunMultipleAppendMemoryCopy(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendImageCopyThenVerifyCopyIsCorrect) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  lzt::zeImageCreateCommon img;
  // dest_host_image_upper is used to validate that the above image copy
  // operation(s) were correct:
  lzt::ImagePNG32Bit dest_host_image_upper(img.dflt_host_image_.width(),
                                           img.dflt_host_image_.height());
  // Scribble a known incorrect data pattern to dest_host_image_upper to
  // ensure we are validating actual data from the L0 functionality:
  lzt::write_image_data_pattern(dest_host_image_upper, -1);

  // First, copy the image from the host to the device:
  lzt::append_image_copy_from_mem(cmdlist_immediate, img.dflt_device_image_2_,
                                  img.dflt_host_image_.raw_data(), event0);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeEventHostSynchronize(event0, timeout);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
    zeEventHostReset(event0);
  }
  // Now, copy the image from the device to the device:
  lzt::append_image_copy(cmdlist_immediate, img.dflt_device_image_,
                         img.dflt_device_image_2_, event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeEventHostSynchronize(event0, timeout);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
    zeEventHostReset(event0);
  }
  // Finally copy the image from the device to the dest_host_image_upper for
  // validation:
  lzt::append_image_copy_to_mem(cmdlist_immediate,
                                dest_host_image_upper.raw_data(),
                                img.dflt_device_image_, event0);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeEventHostSynchronize(event0, timeout);
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
    zeEventHostReset(event0);
  }

  // Validate the result of the above operations:
  // If the operation is a straight image copy, or the second region is null
  // then the result should be the same:
  EXPECT_EQ(0, compare_data_pattern(dest_host_image_upper, img.dflt_host_image_,
                                    0U, 0U, img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height(), 0U, 0U,
                                    img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height()));
}

static void RunAppendImageCopy(ze_command_list_handle_t cmdlist_immediate,
                               ze_command_queue_mode_t mode) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  const uint64_t timeout = UINT64_MAX - 1;
  lzt::zeImageCreateCommon img;
  // dest_host_image_upper is used to validate that the above image copy
  // operation(s) were correct:
  lzt::ImagePNG32Bit dest_host_image_upper(img.dflt_host_image_.width(),
                                           img.dflt_host_image_.height());
  // Scribble a known incorrect data pattern to dest_host_image_upper to
  // ensure we are validating actual data from the L0 functionality:
  lzt::write_image_data_pattern(dest_host_image_upper, -1);

  // First, copy the image from the host to the device:
  lzt::append_image_copy_from_mem(cmdlist_immediate, img.dflt_device_image_2_,
                                  img.dflt_host_image_.raw_data(), nullptr);

  // Now, copy the image from the device to the device:
  lzt::append_image_copy(cmdlist_immediate, img.dflt_device_image_,
                         img.dflt_device_image_2_, nullptr);

  // Finally copy the image from the device to the dest_host_image_upper for
  // validation:
  lzt::append_image_copy_to_mem(cmdlist_immediate,
                                dest_host_image_upper.raw_data(),
                                img.dflt_device_image_, nullptr);

  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  }

  // Validate the result of the above operations:
  // If the operation is a straight image copy, or the second region is null
  // then the result should be the same:
  EXPECT_EQ(0, compare_data_pattern(dest_host_image_upper, img.dflt_host_image_,
                                    0U, 0U, img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height(), 0U, 0U,
                                    img.dflt_host_image_.width(),
                                    img.dflt_host_image_.height()));
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendImageCopyThenVerifyCopyIsCorrect) {
  RunAppendImageCopy(cmdlist_immediate_default_mode,
                     ZE_COMMAND_QUEUE_MODE_DEFAULT);
  RunAppendImageCopy(cmdlist_immediate_sync_mode,
                     ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS);
  RunAppendImageCopy(cmdlist_immediate_async_mode,
                     ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendEventResetThenSuccesIsReturnedAndEventIsReset) {
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  zeEventHostSignal(event0);
  ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendEventReset(cmdlist_immediate, event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event1));
    }
  }
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  ep.destroy_event(event1);
}

static void RunAppendEventReset(ze_command_list_handle_t cmdlist_immediate,
                                ze_command_queue_mode_t mode,
                                lzt::zeEventPool &ep,
                                bool useEventsToSynchronize) {
  const uint64_t timeout = UINT64_MAX - 1;
  ze_event_handle_t event0 = nullptr;
  ze_event_handle_t event1 = nullptr;

  ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);

  zeEventHostSignal(event0);
  ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendEventReset(cmdlist_immediate, event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event1));
    }
  }
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  ep.destroy_event(event1);
  ep.destroy_event(event0);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendEventResetThenSuccesIsReturnedAndEventIsReset) {
  bool useEventsToSynchronize = true;
  for (int i = 0; i < 2; i++) {
    RunAppendEventReset(cmdlist_immediate_default_mode,
                        ZE_COMMAND_QUEUE_MODE_DEFAULT, ep,
                        useEventsToSynchronize);
    RunAppendEventReset(cmdlist_immediate_sync_mode,
                        ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, ep,
                        useEventsToSynchronize);
    RunAppendEventReset(cmdlist_immediate_async_mode,
                        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ep,
                        useEventsToSynchronize);
    useEventsToSynchronize = false;
  }
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendSignalEventThenSuccessIsReturnedAndEventIsSignaled) {
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  ASSERT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendSignalEvent(cmdlist_immediate, event0));
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    }
  }
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  ep.destroy_event(event1);
}

static void RunAppendSignalEvent(ze_command_list_handle_t cmdlist_immediate,
                                 ze_command_queue_mode_t mode,
                                 lzt::zeEventPool &ep,
                                 bool useEventsToSynchronize) {
  const uint64_t timeout = UINT64_MAX - 1;
  ze_event_handle_t event0 = nullptr;
  ze_event_handle_t event1 = nullptr;

  ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);

  ASSERT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendSignalEvent(cmdlist_immediate, event0));
  if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    if (useEventsToSynchronize) {
      zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    }
  }
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  ep.destroy_event(event1);
  ep.destroy_event(event0);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendSignalEventThenSuccessIsReturnedAndEventIsSignaled) {
  bool useEventsToSynchronize = true;
  for (int i = 0; i < 2; i++) {
    RunAppendSignalEvent(cmdlist_immediate_default_mode,
                         ZE_COMMAND_QUEUE_MODE_DEFAULT, ep,
                         useEventsToSynchronize);
    RunAppendSignalEvent(cmdlist_immediate_sync_mode,
                         ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, ep,
                         useEventsToSynchronize);
    RunAppendSignalEvent(cmdlist_immediate_async_mode,
                         ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ep,
                         useEventsToSynchronize);
    useEventsToSynchronize = false;
  }
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendWaitOnEventsThenSuccessIsReturned) {
  if (mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS || !useEventsToSynchronize) {
    GTEST_SKIP();
  }
  ze_event_handle_t event1 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event1));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendWaitOnEvents(cmdlist_immediate, 1, &event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  zeEventHostSignal(event0);
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));

  ep.destroy_event(event1);
}

static void RunAppendWaitOnEvents(ze_command_list_handle_t cmdlist_immediate,
                                  ze_command_queue_mode_t mode,
                                  lzt::zeEventPool &ep,
                                  bool useEventsToSynchronize) {
  const uint64_t timeout = UINT64_MAX - 1;
  ze_event_handle_t event0 = nullptr;
  ze_event_handle_t event1 = nullptr;

  ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event1));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendWaitOnEvents(cmdlist_immediate, 1, &event0));
  zeCommandListAppendBarrier(cmdlist_immediate, event1, 0, nullptr);
  zeEventHostSignal(event0);
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event1, timeout));
  ep.destroy_event(event1);
  ep.destroy_event(event0);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInorderImmediateCommandListWhenAppendWaitOnEventsThenSuccessIsReturned) {
  RunAppendWaitOnEvents(cmdlist_immediate_default_mode,
                        ZE_COMMAND_QUEUE_MODE_DEFAULT, ep, true);
  RunAppendWaitOnEvents(cmdlist_immediate_async_mode,
                        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ep, true);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendMemoryPrefetchThenSuccessIsReturned) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory(size);
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendMemoryPrefetch(cmdlist_immediate, memory, size));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::free_memory(memory);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendMemoryPrefetchThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  size_t size = 4096;
  void *memory = lzt::aligned_malloc(size, 1);
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendMemoryPrefetch(cmdlist_immediate, memory, size));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::aligned_free(memory);
}

static void RunAppendMemoryPrefetch(ze_command_list_handle_t cmdlist_immediate,
                                    const uint64_t timeout,
                                    bool is_shared_system) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory_with_allocator_selector(
      size, is_shared_system);
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendMemoryPrefetch(cmdlist_immediate, memory, size));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendMemoryPrefetchThenSuccessIsReturned) {
  const uint64_t timeout = UINT64_MAX - 1;
  RunAppendMemoryPrefetch(cmdlist_immediate_default_mode, timeout, false);
  RunAppendMemoryPrefetch(cmdlist_immediate_sync_mode, 0, false);
  RunAppendMemoryPrefetch(cmdlist_immediate_async_mode, timeout, false);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendMemoryPrefetchThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  const uint64_t timeout = UINT64_MAX - 1;
  RunAppendMemoryPrefetch(cmdlist_immediate_default_mode, timeout, true);
  RunAppendMemoryPrefetch(cmdlist_immediate_sync_mode, 0, true);
  RunAppendMemoryPrefetch(cmdlist_immediate_async_mode, timeout, true);
}

LZT_TEST_P(zeImmediateCommandListExecutionTests,
           GivenImmediateCommandListWhenAppendMemAdviseThenSuccessIsReturned) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory(size);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(
      cmdlist_immediate, lzt::zeDevice::get_instance()->get_device(), memory,
      size, ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::free_memory(memory);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListWhenAppendMemAdviseThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  size_t size = 4096;
  void *memory = lzt::aligned_malloc(size, 1);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(
      cmdlist_immediate, lzt::zeDevice::get_instance()->get_device(), memory,
      size, ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::aligned_free(memory);
}

static void RunAppendMemAdvise(ze_command_list_handle_t cmdlist_immediate,
                               const uint64_t timeout, bool is_shared_system) {
  size_t size = 4096;
  void *memory = lzt::allocate_shared_memory_with_allocator_selector(
      size, is_shared_system);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(
      cmdlist_immediate, lzt::zeDevice::get_instance()->get_device(), memory,
      size, ZE_MEMORY_ADVICE_SET_READ_MOSTLY));
  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate, timeout));
  lzt::free_memory_with_allocator_selector(memory, is_shared_system);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendMemAdviseThenSuccessIsReturned) {
  const uint64_t timeout = UINT64_MAX - 1;
  RunAppendMemAdvise(cmdlist_immediate_default_mode, timeout, false);
  RunAppendMemAdvise(cmdlist_immediate_sync_mode, 0, false);
  RunAppendMemAdvise(cmdlist_immediate_async_mode, timeout, false);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListWhenAppendMemAdviseThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  const uint64_t timeout = UINT64_MAX - 1;
  RunAppendMemAdvise(cmdlist_immediate_default_mode, timeout, true);
  RunAppendMemAdvise(cmdlist_immediate_sync_mode, 0, true);
  RunAppendMemAdvise(cmdlist_immediate_async_mode, timeout, true);
}

static ze_image_handle_t create_test_image(uint32_t height, uint32_t width) {
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
LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenMultipleImmediateCommandListsThatHaveDependenciesThenAllTheCommandListsExecuteSuccessfully) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  uint32_t width = input.width();
  uint32_t height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  ze_command_list_handle_t cmdlist_immediate_1 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_2 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_3 = nullptr;
  cmdlist_immediate = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_1 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_2 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  cmdlist_immediate_3 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      static_cast<ze_command_queue_flag_t>(0), mode,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  ze_event_handle_t hEvent1, hEvent2, hEvent3, hEvent4;
  ep.create_event(hEvent1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent2, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent3, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(hEvent4, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmdlist_immediate, input_xeimage,
                                  input.raw_data(), hEvent1);
  lzt::append_wait_on_events(cmdlist_immediate_1, 1, &hEvent1);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmdlist_immediate_1, output_xeimage, input_xeimage,
                         hEvent2);
  lzt::append_wait_on_events(cmdlist_immediate_2, 1, &hEvent2);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmdlist_immediate_2, output_xeimage,
                                input_xeimage, &dr, &sr, hEvent3);
  lzt::append_wait_on_events(cmdlist_immediate_3, 1, &hEvent3);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmdlist_immediate_3, output.raw_data(),
                                output_xeimage, hEvent4);
  if (useEventsToSynchronize) {
    lzt::event_host_synchronize(hEvent4, UINT64_MAX);
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmdlist_immediate_3, UINT64_MAX));
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(hEvent4));
  }

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  ep.destroy_event(hEvent1);
  ep.destroy_event(hEvent2);
  ep.destroy_event(hEvent3);
  ep.destroy_event(hEvent4);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
  lzt::destroy_command_list(cmdlist_immediate);
  lzt::destroy_command_list(cmdlist_immediate_1);
  lzt::destroy_command_list(cmdlist_immediate_2);
  lzt::destroy_command_list(cmdlist_immediate_3);
}

static void RunMultipleDependentCommandLists(ze_command_queue_mode_t mode) {
  // create 2 images
  lzt::ImagePNG32Bit input("test_input.png");
  uint32_t width = input.width();
  uint32_t height = input.height();
  lzt::ImagePNG32Bit output(width, height);
  auto input_xeimage = create_test_image(height, width);
  auto output_xeimage = create_test_image(height, width);
  ze_command_list_handle_t cmdlist_immediate = nullptr;
  ze_command_list_handle_t cmdlist_immediate_1 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_2 = nullptr;
  ze_command_list_handle_t cmdlist_immediate_3 = nullptr;
  cmdlist_immediate = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      ZE_COMMAND_QUEUE_FLAG_IN_ORDER, mode, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      0);
  cmdlist_immediate_1 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      ZE_COMMAND_QUEUE_FLAG_IN_ORDER, mode, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      0);
  cmdlist_immediate_2 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      ZE_COMMAND_QUEUE_FLAG_IN_ORDER, mode, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      0);
  cmdlist_immediate_3 = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(),
      ZE_COMMAND_QUEUE_FLAG_IN_ORDER, mode, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
      0);

  // Use ImageCopyFromMemory to upload ImageA
  lzt::append_image_copy_from_mem(cmdlist_immediate, input_xeimage,
                                  input.raw_data(), nullptr);
  // use ImageCopy to copy A -> B
  lzt::append_image_copy(cmdlist_immediate_1, output_xeimage, input_xeimage,
                         nullptr);
  // use ImageCopyRegion to copy part of A -> B
  ze_image_region_t sr = {0, 0, 0, 1, 1, 1};
  ze_image_region_t dr = {0, 0, 0, 1, 1, 1};
  lzt::append_image_copy_region(cmdlist_immediate_2, output_xeimage,
                                input_xeimage, &dr, &sr, nullptr);
  // use ImageCopyToMemory to dowload ImageB
  lzt::append_image_copy_to_mem(cmdlist_immediate_3, output.raw_data(),
                                output_xeimage, nullptr);

  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListHostSynchronize(cmdlist_immediate_3, UINT64_MAX));

  // Verfy A matches B
  EXPECT_EQ(0,
            memcmp(input.raw_data(), output.raw_data(), input.size_in_bytes()));

  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage);
  lzt::destroy_command_list(cmdlist_immediate);
  lzt::destroy_command_list(cmdlist_immediate_1);
  lzt::destroy_command_list(cmdlist_immediate_2);
  lzt::destroy_command_list(cmdlist_immediate_3);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenMultipleInOrderImmediateCommandListsThatHaveDependenciesThenAllTheCommandListsExecuteSuccessfully) {
  if (!(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  RunMultipleDependentCommandLists(ZE_COMMAND_QUEUE_MODE_DEFAULT);
  RunMultipleDependentCommandLists(ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS);
  RunMultipleDependentCommandLists(ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS);
}

LZT_TEST_P(
    zeImmediateCommandListExecutionTests,
    GivenImmediateCommandListAndRegularCommandListWithSameIndexWhenAppendingOperationsThenVerifyOperationsAreSuccessful) {
  const size_t size = 16;
  const int num_iteration = 3;
  int addval = 10;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event0));

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int *p_dev = static_cast<int *>(buffer);
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = to_u32(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  auto command_queue = lzt::create_command_queue();
  auto command_list = lzt::create_command_list();
  const size_t bufsize = 4096;
  std::vector<uint8_t> host_memory1(bufsize), host_memory2(bufsize, 0);
  void *device_memory =
      lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), bufsize, 1);

  ze_event_handle_t event1 = nullptr;
  ze_event_handle_t event2 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(event2, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  lzt::append_memory_copy(command_list, device_memory, host_memory1.data(),
                          lzt::size_in_bytes(host_memory1), event1);

  lzt::append_memory_copy(command_list, host_memory2.data(), device_memory,
                          lzt::size_in_bytes(host_memory2), event2, 1, &event1);

  lzt::close_command_list(command_list);

  int val_check = addval;
  for (uint32_t iter = 0; iter < num_iteration; iter++) {
    lzt::append_launch_function(cmdlist_immediate, kernel, &tg, event0, 0,
                                nullptr);

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

    // Synchronize executions
    if (useEventsToSynchronize) {
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    }

    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event0));
    lzt::event_host_synchronize(event2, UINT64_MAX);
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event1));
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event2));

    // Validate kernel and copy execution
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[i], val_check);
    }

    addval += 5;
    val_check += addval;
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    lzt::validate_data_pattern(host_memory2.data(), bufsize, 1);

    // Reset buffers
    memset(host_memory2.data(), 0x0, bufsize * sizeof(uint8_t));

    uint8_t zero = 0;
    lzt::append_memory_set(cmdlist_immediate, device_memory, &zero,
                           bufsize * sizeof(uint8_t), event0);
    if (useEventsToSynchronize) {
      EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event0, timeout));
    } else {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event0));
    }
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event0));
  }

  lzt::free_memory(device_memory);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::free_memory(buffer);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

static void
RunIclAndRclAppendOperations(ze_command_list_handle_t cmdlist_immediate,
                             ze_command_queue_mode_t mode,
                             lzt::zeEventPool &ep) {
  const uint64_t timeout = UINT64_MAX - 1;
  const size_t size = 16;
  const int num_iteration = 3;
  int addval = 10;

  void *buffer = lzt::allocate_shared_memory(size * sizeof(int));
  memset(buffer, 0x0, size * sizeof(int));

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int *p_dev = static_cast<int *>(buffer);
  lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  ze_group_count_t tg;
  tg.groupCountX = to_u32(size);
  tg.groupCountY = 1;
  tg.groupCountZ = 1;

  auto command_queue = lzt::create_command_queue();
  auto command_list = lzt::create_command_list();
  const size_t bufsize = 4096;
  std::vector<uint8_t> host_memory1(bufsize), host_memory2(bufsize, 0);
  void *device_memory =
      lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), bufsize, 1);

  ze_event_handle_t event1 = nullptr;
  ze_event_handle_t event2 = nullptr;
  ep.create_event(event1, ZE_EVENT_SCOPE_FLAG_HOST, 0);
  ep.create_event(event2, ZE_EVENT_SCOPE_FLAG_HOST, 0);

  lzt::append_memory_copy(command_list, device_memory, host_memory1.data(),
                          lzt::size_in_bytes(host_memory1), event1);

  lzt::append_memory_copy(command_list, host_memory2.data(), device_memory,
                          lzt::size_in_bytes(host_memory2), event2, 1, &event1);

  lzt::close_command_list(command_list);

  int val_check = addval;
  for (uint32_t iter = 0; iter < num_iteration; iter++) {
    lzt::append_launch_function(cmdlist_immediate, kernel, &tg, nullptr, 0,
                                nullptr);

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

    lzt::event_host_synchronize(event2, UINT64_MAX);
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event1));
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostReset(event2));

    // Validate kernel and copy execution
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[i], val_check);
    }

    addval += 5;
    val_check += addval;
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    lzt::validate_data_pattern(host_memory2.data(), bufsize, 1);

    // Reset buffers
    memset(host_memory2.data(), 0x0, bufsize * sizeof(uint8_t));

    uint8_t zero = 0;
    lzt::append_memory_set(cmdlist_immediate, device_memory, &zero,
                           bufsize * sizeof(uint8_t), nullptr);
    if (mode != ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
      EXPECT_ZE_RESULT_SUCCESS(
          zeCommandListHostSynchronize(cmdlist_immediate, timeout));
    }
  }

  lzt::free_memory(device_memory);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::free_memory(buffer);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

LZT_TEST_F(
    zeImmediateCommandListInOrderExecutionTests,
    GivenInOrderImmediateCommandListAndRegularCommandListWithSameIndexWhenAppendingOperationsThenVerifyOperationsAreSuccessful) {
  RunIclAndRclAppendOperations(cmdlist_immediate_default_mode,
                               ZE_COMMAND_QUEUE_MODE_DEFAULT, ep);
  RunIclAndRclAppendOperations(cmdlist_immediate_sync_mode,
                               ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS, ep);
  RunIclAndRclAppendOperations(cmdlist_immediate_async_mode,
                               ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ep);
}

INSTANTIATE_TEST_SUITE_P(
    TestCasesforCommandListImmediateCases, zeImmediateCommandListExecutionTests,
    testing::Combine(testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                     ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                                     ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
                     testing::Values(true, false)));

class zeImmediateCommandListHostSynchronizeTimeoutTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_event_pool_flags_t, ze_command_queue_flags_t, bool>> {
protected:
  void SetUp() override {
    const auto ep_flags =
        std::get<0>(GetParam()) | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    const auto cq_flags = std::get<1>(GetParam());

    auto context = lzt::get_default_context();
    auto device = lzt::get_default_device(lzt::get_default_driver());

    ze_event_pool_desc_t ep_desc{};
    ep_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    ep_desc.pNext = nullptr;
    ep_desc.flags = ep_flags;
    ep_desc.count = 1;
    ep = lzt::create_event_pool(context, ep_desc);

    ze_event_desc_t ev_desc{};
    ev_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    ev_desc.pNext = nullptr;
    ev_desc.index = 0;
    ev_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    ev_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ev = lzt::create_event(ep, ev_desc);

    cl = lzt::create_immediate_command_list(
        context, device, cq_flags, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);

    const uint64_t responsiveness = measure_device_responsiveness();
    const double min_ratio = 0.02;
    if (responsiveness > to_u64(min_ratio * to_f64(timeout))) {
      timeout = to_u64(to_f64(responsiveness) / min_ratio);
      LOG_INFO << "Device responsiveness: " << responsiveness
               << " ns, setting timeout to: " << timeout << " ns";
    }
  }

  uint64_t measure_device_responsiveness() {
    const ze_context_handle_t context =
        lzt::create_context(lzt::get_default_driver());
    const ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());
    ze_command_list_handle_t cmd_list = lzt::create_immediate_command_list(
        device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
        0);

    ze_module_handle_t module =
        lzt::create_module(context, device, "cmdlist_add.spv",
                           ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
    ze_kernel_handle_t kernel =
        lzt::create_function(module, "cmdlist_add_constant");

    size_t size = 10000;
    size_t buff_size = size * sizeof(int);
    int *buf_hst =
        static_cast<int *>(lzt::allocate_host_memory(buff_size, 1, context));
    int *buf_dev = static_cast<int *>(
        lzt::allocate_device_memory(buff_size, 1, 0, 0, device, context));
    std::fill_n(buf_hst, size, 0);

    const int add_value = 7;
    lzt::set_group_size(kernel, 1, 1, 1);
    ze_group_count_t args = {to_u32(size), 1, 1};
    lzt::set_argument_value(kernel, 0, sizeof(buf_dev), &buf_dev);
    lzt::set_argument_value(kernel, 1, sizeof(add_value), &add_value);

    lzt::append_memory_copy(cmd_list, buf_dev, buf_hst, buff_size, nullptr, 0,
                            nullptr);

    const auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < 2; ++i) {
      lzt::append_launch_function(cmd_list, kernel, &args, nullptr, 0, nullptr);
    }
    const auto t1 = std::chrono::steady_clock::now();

    lzt::append_memory_copy(cmd_list, buf_hst, buf_dev, buff_size, nullptr, 0,
                            nullptr);
    lzt::synchronize_command_list_host(cmd_list,
                                       std::numeric_limits<uint64_t>::max());

    // Verify
    for (size_t i = 0; i < size; ++i) {
      EXPECT_EQ(buf_hst[i], 2 * add_value);
    }

    // Cleanup
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::free_memory(context, buf_dev);
    lzt::free_memory(context, buf_hst);
    lzt::destroy_command_list(cmd_list);
    lzt::destroy_context(context);

    return to_nanoseconds(t1 - t0);
  }

  void TearDown() override {
    lzt::destroy_event(ev);
    lzt::destroy_event_pool(ep);
    lzt::destroy_command_list(cl);
  }

  uint64_t timeout = 5000000;
  ze_event_pool_handle_t ep = nullptr;
  ze_event_handle_t ev = nullptr;
  ze_command_list_handle_t cl = nullptr;
};

LZT_TEST_P(
    zeImmediateCommandListHostSynchronizeTimeoutTests,
    GivenTimeoutWhenWaitingForImmediateCommandListThenWaitForSpecifiedTime) {
  const bool use_barrier = std::get<2>(GetParam());

  if (use_barrier) {
    lzt::append_barrier(cl, nullptr, 1, &ev);
  } else {
    lzt::append_wait_on_events(cl, 1, &ev);
  }

  const auto t0 = std::chrono::steady_clock::now();
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeCommandListHostSynchronize(cl, timeout));
  const auto t1 = std::chrono::steady_clock::now();

  const uint64_t wall_time = to_nanoseconds(t1 - t0);
  const float ratio = to_f32(wall_time) / to_f32(timeout);
  // Tolerance: 2%
  EXPECT_GE(ratio, 0.98f);
  EXPECT_LE(ratio, 1.02f);

  lzt::signal_event_from_host(ev);
  lzt::synchronize_command_list_host(cl, UINT64_MAX);
  lzt::event_host_synchronize(ev, UINT64_MAX);
}

INSTANTIATE_TEST_SUITE_P(
    SyncTimeoutParams, zeImmediateCommandListHostSynchronizeTimeoutTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_EVENT_POOL_FLAG_IPC,
                          ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_IPC |
                              ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
                          ZE_EVENT_POOL_FLAG_IPC |
                              ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP),
        ::testing::Values(0, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
                          ZE_COMMAND_QUEUE_FLAG_IN_ORDER),
        ::testing::Bool()));

} // namespace
