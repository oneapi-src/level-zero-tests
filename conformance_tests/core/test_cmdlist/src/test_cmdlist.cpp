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
#include "logging/logging.hpp"
#include "random/random.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

#include <chrono>
#include <bitset>

namespace {

using cmdListVec = std::vector<ze_command_list_handle_t>;
using cmdQueueVec = std::vector<ze_command_queue_handle_t>;

class zeCommandListCreateTests : public ::testing::Test,
                                 public ::testing::WithParamInterface<
                                     std::tuple<ze_command_list_flag_t, bool>> {
};

LZT_TEST_P(
    zeCommandListCreateTests,
    GivenValidDeviceAndCommandListDescriptorWhenCreatingCommandListThenNotNullCommandListIsReturned) {

  auto bundle = lzt::create_command_bundle(
      lzt::zeDevice::get_instance()->get_device(), std::get<0>(GetParam()),
      std::get<1>(GetParam()));
  EXPECT_NE(nullptr, bundle.list);

  lzt::destroy_command_bundle(bundle);
}

INSTANTIATE_TEST_SUITE_P(
    CreateFlagParameterizedTest, zeCommandListCreateTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY,
                          ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_FORCE_UINT32),
        ::testing::Bool()));

class zeCommandListDestroyTests : public ::testing::Test {
public:
  void run(bool is_immediate) {
    auto bundle = lzt::create_command_bundle(
        lzt::zeDevice::get_instance()->get_device(), is_immediate);
    EXPECT_NE(nullptr, bundle.list);

    lzt::destroy_command_bundle(bundle);
  }
};

LZT_TEST_F(
    zeCommandListDestroyTests,
    GivenValidDeviceAndCommandListDescriptorWhenDestroyingCommandListThenSuccessIsReturned) {
  run(false);
}

LZT_TEST_F(
    zeCommandListDestroyTests,
    GivenValidDeviceAndImmediateCommandListDescriptorWhenDestroyingCommandListThenSuccessIsReturned) {
  run(true);
}

class zeCommandListCreateImmediateTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flag_t, ze_command_queue_mode_t,
                     ze_command_queue_priority_t>> {};

LZT_TEST_P(
    zeCommandListCreateImmediateTests,
    GivenImplicitCommandQueueWhenCreatingCommandListThenSuccessIsReturned) {

  ze_command_queue_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
  descriptor.flags = std::get<0>(GetParam());
  descriptor.mode = std::get<1>(GetParam());
  descriptor.priority = std::get<2>(GetParam());

  ze_command_list_handle_t command_list = nullptr;
  command_list = lzt::create_immediate_command_list(
      lzt::zeDevice::get_instance()->get_device(), std::get<0>(GetParam()),
      std::get<1>(GetParam()), std::get<2>(GetParam()), 0);
  lzt::destroy_command_list(command_list);
}

INSTANTIATE_TEST_SUITE_P(
    ImplictCommandQueueParameterizedTest, zeCommandListCreateImmediateTests,
    ::testing::Combine(
        ::testing::Values(static_cast<ze_command_queue_flag_t>(0),
                          ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW,
                          ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH)));

class zeCommandListCloseTests : public ::testing::Test {
public:
  void run(const bool is_immediate) {
    auto bundle = lzt::create_command_bundle(is_immediate);
    EXPECT_NE(nullptr, bundle.list);
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListClose(bundle.list));
    lzt::destroy_command_bundle(bundle);
  }
};

LZT_TEST_F(zeCommandListCloseTests,
           GivenEmptyCommandListWhenClosingCommandListThenSuccessIsReturned) {
  run(false);
}

LZT_TEST_F(
    zeCommandListCloseTests,
    GivenEmptyImmediateCommandListWhenClosingCommandListThenSuccessIsReturned) {
  run(true);
}

class zeCommandListResetTests : public ::testing::Test {
protected:
  void run_reset_test(bool execute_all_commands, bool is_immediate) {
    auto device = lzt::zeDevice::get_instance()->get_device();
    auto bundle = lzt::create_command_bundle(is_immediate);

    const size_t size = 16;
    std::vector<uint8_t> host_memory1(size);
    std::vector<uint8_t> host_memory2(size);
    auto device_dest = lzt::allocate_device_memory(size);
    auto device_src = lzt::allocate_device_memory(size);
    auto host_mem = lzt::allocate_host_memory(size);

    // Append various operations to the command list
    lzt::write_data_pattern(host_memory1.data(), size, 0);
    lzt::append_memory_copy(bundle.list, device_dest, host_memory1.data(),
                            size);
    lzt::append_barrier(bundle.list);
    lzt::write_data_pattern(host_memory2.data(), size, 1);
    lzt::append_memory_copy(bundle.list, device_src, host_memory2.data(), size);
    lzt::append_barrier(bundle.list);
    lzt::write_data_pattern(host_mem, size, -1);
    lzt::append_memory_copy(bundle.list, device_dest, device_src, size);
    lzt::append_barrier(bundle.list);
    lzt::append_memory_copy(bundle.list, host_mem, device_dest, size);
    lzt::append_barrier(bundle.list);

    uint8_t pattern = 0xAA;
    int pattern_size = 1;
    auto memory_fill_mem = lzt::allocate_shared_memory(size);
    memset(memory_fill_mem, 0, size);
    lzt::append_memory_fill(bundle.list, memory_fill_mem, &pattern,
                            pattern_size, size, nullptr);
    lzt::append_barrier(bundle.list);

    lzt::zeImageCreateCommon *img_ptr = nullptr;
    lzt::ImagePNG32Bit *dest_host_image_ptr = nullptr;

    if (lzt::image_support()) {
      img_ptr = new lzt::zeImageCreateCommon;
      dest_host_image_ptr =
          new lzt::ImagePNG32Bit(img_ptr->dflt_host_image_.width(),
                                 img_ptr->dflt_host_image_.height());
      lzt::write_image_data_pattern(*dest_host_image_ptr, -1);
      lzt::append_image_copy_from_mem(bundle.list, img_ptr->dflt_device_image_,
                                      img_ptr->dflt_host_image_.raw_data(),
                                      nullptr);
      lzt::append_barrier(bundle.list);
      lzt::append_image_copy_to_mem(bundle.list,
                                    dest_host_image_ptr->raw_data(),
                                    img_ptr->dflt_device_image_, nullptr);
    }
    ze_module_handle_t module = lzt::create_module(device, "cmdlist_add.spv");
    ze_kernel_handle_t kernel =
        lzt::create_function(module, "cmdlist_add_constant");
    lzt::set_group_size(kernel, 1, 1, 1);
    ze_group_count_t args = {static_cast<uint32_t>(size), 1, 1};

    const int addval = 10;
    auto kernel_buffer = lzt::allocate_shared_memory(size * sizeof(int));
    memset(kernel_buffer, 0, size * sizeof(int));
    lzt::set_argument_value(kernel, 0, sizeof(kernel_buffer), &kernel_buffer);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    lzt::append_launch_function(bundle.list, kernel, &args, nullptr, 0,
                                nullptr);

    ze_event_handle_t event = nullptr;
    ze_event_pool_desc_t event_pool_desc = {};
    event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    event_pool_desc.count = 1;
    ze_event_desc_t event_desc = {};
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.index = 0;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    lzt::zeEventPool ep;
    ep.InitEventPool(event_pool_desc);
    ep.create_event(event, event_desc);
    lzt::append_signal_event(bundle.list, event);

    lzt::close_command_list(bundle.list);
    if (is_immediate) {
      lzt::synchronize_command_list_host(bundle.list, UINT64_MAX);
    } else if (execute_all_commands) {
      lzt::execute_command_lists(bundle.queue, 1, &bundle.list, nullptr);
      lzt::synchronize(bundle.queue, UINT64_MAX);
    }

    lzt::reset_command_list(bundle.list);

    auto test_mem = lzt::allocate_shared_memory(size);
    memset(test_mem, 0, size);
    lzt::append_memory_fill(bundle.list, test_mem, &pattern, sizeof(uint8_t),
                            size, nullptr);
    lzt::close_command_list(bundle.list);
    lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

    if (execute_all_commands || is_immediate) {
      lzt::validate_data_pattern(host_mem, size, 1);
      if (lzt::image_support()) {
        EXPECT_EQ(0, compare_data_pattern(*dest_host_image_ptr,
                                          img_ptr->dflt_host_image_, 0, 0,
                                          img_ptr->dflt_host_image_.width(),
                                          img_ptr->dflt_host_image_.height(), 0,
                                          0, img_ptr->dflt_host_image_.width(),
                                          img_ptr->dflt_host_image_.height()));
        delete img_ptr;
        delete dest_host_image_ptr;
      }
      for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(static_cast<int *>(kernel_buffer)[i], addval);
        EXPECT_EQ(static_cast<uint8_t *>(memory_fill_mem)[i], pattern);
      }
      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event));
    } else {
      lzt::validate_data_pattern(host_mem, size, -1);
      for (size_t i = 0; i < size; i++) {
        EXPECT_EQ(static_cast<int *>(kernel_buffer)[i], 0);
        EXPECT_EQ(static_cast<uint8_t *>(memory_fill_mem)[i], 0);
      }
      if (lzt::image_support()) {
        EXPECT_NE(0, compare_data_pattern(*dest_host_image_ptr,
                                          img_ptr->dflt_host_image_, 0, 0,
                                          img_ptr->dflt_host_image_.width(),
                                          img_ptr->dflt_host_image_.height(), 0,
                                          0, img_ptr->dflt_host_image_.width(),
                                          img_ptr->dflt_host_image_.height()));
        delete img_ptr;
        delete dest_host_image_ptr;
      }
      EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(event));
    }

    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<uint8_t *>(test_mem)[i], pattern);
    }

    // cleanup
    ep.destroy_event(event);
    lzt::free_memory(device_dest);
    lzt::free_memory(device_src);
    lzt::free_memory(host_mem);
    lzt::free_memory(memory_fill_mem);
    lzt::free_memory(kernel_buffer);
    lzt::free_memory(test_mem);
    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_bundle(bundle);
  }
};

LZT_TEST_F(zeCommandListResetTests,
           GivenEmptyCommandListWhenResettingCommandListThenSuccessIsReturned) {
  auto bundle = lzt::create_command_bundle(false);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListReset(bundle.list));
  lzt::destroy_command_bundle(bundle);
}

LZT_TEST_F(
    zeCommandListResetTests,
    GivenEmptyImmediateCommandListWhenResettingCommandListThenSuccessIsReturned) {
  auto bundle = lzt::create_command_bundle(true);
  EXPECT_ZE_RESULT_SUCCESS(zeCommandListReset(bundle.list));
  lzt::destroy_command_bundle(bundle);
}

LZT_TEST_F(
    zeCommandListResetTests,
    GivenResetCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenOnlyMemoryFillExecuted) {
  run_reset_test(false, false);
}

LZT_TEST_F(
    zeCommandListResetTests,
    GivenResetImmediateCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenOnlyMemoryFillExecuted) {
  run_reset_test(false, true);
}

LZT_TEST_F(
    zeCommandListResetTests,
    GivenResetExecutedCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenAllCommandsExecuted) {
  run_reset_test(true, false);
}

LZT_TEST_F(
    zeCommandListResetTests,
    GivenResetExecutedImmediateCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenAllCommandsExecuted) {
  run_reset_test(true, true);
}

class zeCommandListReuseTests : public ::testing::Test {};

LZT_TEST(zeCommandListReuseTests,
         GivenCommandListWhenItIsExecutedItCanBeRunAgain) {
  auto cmdlist_mem_set = lzt::create_command_list();
  auto cmdlist_mem_zero = lzt::create_command_list();
  auto cmdq = lzt::create_command_queue();
  const size_t size = 16;
  auto device_buffer = lzt::allocate_device_memory(size);
  auto host_buffer = lzt::allocate_host_memory(size);
  const uint8_t one = 1;
  const uint8_t zero = 0;

  lzt::append_memory_set(cmdlist_mem_set, device_buffer, &one, size);
  lzt::append_barrier(cmdlist_mem_set);
  lzt::append_memory_copy(cmdlist_mem_set, host_buffer, device_buffer, size);
  lzt::close_command_list(cmdlist_mem_set);
  lzt::append_memory_set(cmdlist_mem_zero, device_buffer, &zero, size);
  lzt::append_barrier(cmdlist_mem_zero);
  lzt::append_memory_copy(cmdlist_mem_zero, host_buffer, device_buffer, size);
  lzt::close_command_list(cmdlist_mem_zero);

  const int num_execute = 5;
  for (int i = 0; i < num_execute; i++) {
    lzt::execute_command_lists(cmdq, 1, &cmdlist_mem_zero, nullptr);
    lzt::synchronize(cmdq, UINT64_MAX);
    for (int j = 0; j < size; j++)
      ASSERT_EQ(static_cast<uint8_t *>(host_buffer)[j], 0x0)
          << "Memory Set did not match.";

    lzt::execute_command_lists(cmdq, 1, &cmdlist_mem_set, nullptr);
    lzt::synchronize(cmdq, UINT64_MAX);
    for (int j = 0; j < size; j++)
      ASSERT_EQ(static_cast<uint8_t *>(host_buffer)[j], 0x1)
          << "Memory Set did not match.";
  }

  lzt::destroy_command_list(cmdlist_mem_set);
  lzt::destroy_command_list(cmdlist_mem_zero);
  lzt::destroy_command_queue(cmdq);
  lzt::free_memory(device_buffer);
  lzt::free_memory(host_buffer);
}

class zeCommandListCloseThenAppendTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_list_flag_t> {};

void RunGivenResetCommandListWhenCloseImmediatelyNoInstructionsExecute(
    ze_command_list_flag_t flags) {
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  auto cmd_queue = lzt::create_command_queue(device);
  auto cmd_list = lzt::create_command_list(device, flags);

  const size_t size = 16;
  auto buffer = lzt::allocate_shared_memory(size);
  const uint8_t set_succeed_1 = 0x1;
  const uint8_t set_fail_2 = 0x2;
  const uint8_t set_fail_3 = 0x3;

  memset(buffer, 0x0, size);
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], 0x0);
  }
  // Command list setup with only memory set and barrier
  lzt::append_memory_set(cmd_list, buffer, &set_succeed_1, size);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_list);
  // Attempt to append command list after close should fail
  lzt::append_memory_set(cmd_list, buffer, &set_fail_2, size);
  lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_queue, UINT64_MAX);
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], set_succeed_1);
  }

  memset(buffer, 0x0, size);
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], 0x0);
  }
  // Reset command list and immediately close
  lzt::reset_command_list(cmd_list);
  lzt::close_command_list(cmd_list);
  // Attempt to append command list after close should fail
  lzt::append_memory_set(cmd_list, buffer, &set_fail_3, size);
  lzt::execute_command_lists(cmd_queue, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_queue, UINT64_MAX);
  // No commands should be executed by command queue
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], 0x0);
  }
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_command_queue(cmd_queue);
  lzt::free_memory(buffer);
}

LZT_TEST_P(
    zeCommandListCloseThenAppendTests,
    GivenResetCommandListWhenCloseImmediatelyNoCommandListInstructionsExecute) {
  RunGivenResetCommandListWhenCloseImmediatelyNoInstructionsExecute(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    TestCasesforCommandListCloseAndCommandListAppend,
    zeCommandListCloseThenAppendTests,
    ::testing::Values(0, ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                      ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                      ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY));

class zeCommandListCloseAndResetTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_list_flag_t, bool>> {};

void RunWhenResetThenVerifyOnlySubsequentInstructionsExecuted(
    ze_command_list_flag_t flags, bool is_immediate) {
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  auto bundle = lzt::create_command_bundle(device, flags, is_immediate);
  const size_t num_instr = 8;
  const size_t size = 16;

  std::vector<void *> buffer;
  std::vector<uint8_t> val;
  for (size_t i = 0; i < num_instr; i++) {
    buffer.push_back(lzt::allocate_shared_memory(size));
    val.push_back(static_cast<uint8_t>(i + 1));
  }

  // Begin with num_instr command list instructions, reset and reduce by one
  // each time.
  size_t test_instr = num_instr;

  while (test_instr) {
    for (auto buf : buffer) {
      memset(buf, 0x0, size);
    }
    for (size_t i = 0; i < test_instr; i++) {
      lzt::append_memory_set(bundle.list, buffer[i], &val[test_instr - (i + 1)],
                             size);
    }
    lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(bundle.list);
    lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
    for (size_t i = 0; i < test_instr; i++) {
      for (size_t j = 0; j < size; j++) {
        EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j],
                  val[test_instr - (i + 1)]);
      }
    }
    for (size_t i = test_instr; i < num_instr; i++) {
      for (size_t j = 0; j < size; j++) {
        EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], 0x0);
      }
    }
    lzt::reset_command_list(bundle.list);
    test_instr--;
  }
  // Last check:  no instructions should be executed
  for (auto buf : buffer) {
    memset(buf, 0x0, size);
  }
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  for (size_t i = 0; i < num_instr; i++) {
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], 0x0);
    }
  }

  // Command list setup with only memory set and barrier

  lzt::destroy_command_bundle(bundle);
  for (auto buf : buffer) {
    lzt::free_memory(buf);
  }
}

LZT_TEST_P(
    zeCommandListCloseAndResetTests,
    GivenCommandListWhenResetThenVerifyOnlySubsequentInstructionsExecuted) {
  RunWhenResetThenVerifyOnlySubsequentInstructionsExecuted(
      std::get<0>(GetParam()), std::get<1>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    TestCasesforCommandListCloseAndCommandListReset,
    zeCommandListCloseAndResetTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY),
        ::testing::Bool()));

class zeCommandListFlagTests : public ::testing::Test,
                               public ::testing::WithParamInterface<
                                   std::tuple<ze_command_list_flag_t, bool>> {};
LZT_TEST_P(zeCommandListFlagTests,
           GivenCommandListCreatedWithDifferentFlagsThenSuccessIsReturned) {
  size_t size = 1024;
  auto device_memory = lzt::allocate_device_memory(size);
  auto host_memory = lzt::allocate_host_memory(size);
  auto device = lzt::zeDevice::get_instance()->get_device();
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  ze_command_list_flags_t flags = std::get<0>(GetParam());
  bool is_immediate = std::get<1>(GetParam());
  auto cmd_bundle = lzt::create_command_bundle(device, flags, is_immediate);

  lzt::append_memory_fill(cmd_bundle.list, device_memory, &pattern,
                          pattern_size, size, nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(cmd_bundle.list, host_memory, device_memory, size,
                          nullptr);
  lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(host_memory)[i], pattern);
  }
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::free_memory(host_memory);
  lzt::free_memory(device_memory);
}

INSTANTIATE_TEST_SUITE_P(
    CreateFlagParameterizedTest, zeCommandListFlagTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                          ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                          ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY),
        ::testing::Bool()));

static void
RunGivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopTest(
    bool is_shared_system_src, bool is_shared_system_dst) {
  auto cq = lzt::create_command_queue();
  auto cmd_list = lzt::create_command_list();
  const int transfer_size = 1;
  const int num_iteration = 3;
  const uint8_t max_copy_count = 100;
  const size_t copy_size = 100;
  auto host_memory_src = lzt::allocate_host_memory_with_allocator_selector(
      copy_size, is_shared_system_src);
  auto host_memory_dst = lzt::allocate_host_memory_with_allocator_selector(
      copy_size, is_shared_system_dst);

  auto temp = static_cast<uint8_t *>(host_memory_src);
  for (uint8_t i = 0; i < max_copy_count; i++) {
    temp[i] = i;
  }

  auto temp1 = static_cast<uint8_t *>(host_memory_src);
  auto temp2 = static_cast<uint8_t *>(host_memory_dst);
  for (uint32_t i = 0; i < max_copy_count; i++) {
    lzt::append_memory_copy(cmd_list, temp2, temp1, transfer_size, nullptr);
    temp1++;
    temp2++;
  }
  lzt::close_command_list(cmd_list);
  for (uint32_t iter = 0; iter < num_iteration; iter++) {
    lzt::execute_command_lists(cq, 1, &cmd_list, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
  }
  lzt::reset_command_list(cmd_list);

  temp = static_cast<uint8_t *>(host_memory_dst);
  for (uint32_t i = 0; i < max_copy_count; i++) {
    ASSERT_EQ(temp[i], i);
  }

  for (uint32_t iter = 0; iter < num_iteration; iter++) {
    temp1 = static_cast<uint8_t *>(host_memory_src);
    temp2 = static_cast<uint8_t *>(host_memory_dst);
    for (uint32_t i = 0; i < max_copy_count; i++) {
      temp1[i] = i;
      temp2[i] = 0;
    }
    for (uint32_t i = 0; i < max_copy_count; i++) {
      lzt::append_memory_copy(cmd_list, temp2, temp1, transfer_size, nullptr);
      temp1++;
      temp2++;
    }
    lzt::close_command_list(cmd_list);
    lzt::execute_command_lists(cq, 1, &cmd_list, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cmd_list);

    temp = static_cast<uint8_t *>(host_memory_dst);
    for (uint32_t i = 0; i < max_copy_count; i++) {
      ASSERT_EQ(temp[i], i);
    }
  }

  lzt::destroy_command_list(cmd_list);
  lzt::destroy_command_queue(cq);
  lzt::free_memory_with_allocator_selector(host_memory_src,
                                           is_shared_system_src);
  lzt::free_memory_with_allocator_selector(host_memory_dst,
                                           is_shared_system_dst);
}

LZT_TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopThenSuccessIsReturned) {
  RunGivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopTest(
      false, false);
}

LZT_TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  for (int i = 1; i < 4; ++i) {
    std::bitset<2> bits(i);
    RunGivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopTest(
        bits[1], bits[0]);
  }
}

void RunAppendingWriteGlobalTimestampThenSuccessIsReturned(bool is_immediate) {
  ze_event_handle_t event = nullptr;
  ze_event_pool_desc_t event_pool_desc = {};
  event_pool_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
  event_pool_desc.count = 1;
  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  lzt::zeEventPool ep;
  ep.InitEventPool(event_pool_desc);
  ep.create_event(event, event_desc);

  auto bundle = lzt::create_command_bundle(is_immediate);
  uint64_t dst_timestamp;
  auto wait_event_initial = event;

  ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(
      bundle.list, &dst_timestamp, nullptr, 1, &event));
  ASSERT_EQ(event, wait_event_initial);

  lzt::close_command_list(bundle.list);
  lzt::signal_event_from_host(event);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

  ep.destroy_event(event);
  lzt::destroy_command_bundle(bundle);
}

LZT_TEST(
    zeCommandListAppendWriteGlobalTimestampTest,
    GivenCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned) {
  RunAppendingWriteGlobalTimestampThenSuccessIsReturned(false);
}

LZT_TEST(
    zeCommandListAppendWriteGlobalTimestampTest,
    GivenImmediateCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned) {
  RunAppendingWriteGlobalTimestampThenSuccessIsReturned(true);
}

static void
RunGivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturnedTest(
    bool is_shared_system_scratch_kernel, bool is_shared_system_src,
    bool is_shared_system_dst) {
  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  uint32_t array_size = 32;
  uint32_t type_size = sizeof(uint32_t);

  ze_module_handle_t module_handle = nullptr;
  ze_kernel_handle_t scratch_function = nullptr;

  for (;;) {
    if (scratch_function)
      lzt::destroy_function(scratch_function);
    if (module_handle)
      lzt::destroy_module(module_handle);
    module_handle = lzt::create_module(
        device, "cmdlist_scratch_" + std::to_string(array_size) + ".spv");
    ze_kernel_flags_t flag = 0;
    scratch_function = lzt::create_function(module_handle, flag, "spill_test");

    auto kernel_properties = lzt::get_kernel_properties(scratch_function);
    if (kernel_properties.spillMemSize != 0u) {
      LOG_INFO << "Buffer size (elements) = " << array_size
               << " Scratch size (bytes) = " << kernel_properties.spillMemSize;
      break;
    }

    if (array_size < 96)
      array_size += 16;
    else if (array_size < 192)
      array_size += 32;
    else if (array_size < 256)
      array_size += 64;
    else if (array_size < 1536)
      array_size += 256;
    else
      throw std::runtime_error("SIZE bigger than 1536 is not supported.");
  }

  uint32_t in_memory_size = array_size * type_size;
  uint32_t out_memory_size = in_memory_size * 2;
  uint32_t offset_memory_size = in_memory_size;
  uint32_t expected_memory_size = in_memory_size * 2;

  void *src_buffer = lzt::allocate_host_memory_with_allocator_selector(
      in_memory_size, is_shared_system_scratch_kernel);
  void *dst_buffer = lzt::allocate_host_memory_with_allocator_selector(
      out_memory_size, is_shared_system_scratch_kernel);
  void *offset_buffer = lzt::allocate_host_memory_with_allocator_selector(
      offset_memory_size, is_shared_system_scratch_kernel);
  void *expected_memory = lzt::allocate_host_memory_with_allocator_selector(
      expected_memory_size, is_shared_system_scratch_kernel);

  // create two buffers for append fill/copy kernel
  size_t size = 1536;
  void *device_memory = lzt::allocate_device_memory_with_allocator_selector(
      size, is_shared_system_src);
  void *host_memory = lzt::allocate_host_memory_with_allocator_selector(
      size, is_shared_system_dst);

  uint8_t pattern = 0xAB;
  size_t pattern_size = 1;
  uint32_t num_iterations = 2;

  auto cmd_list0 = lzt::create_command_list();
  auto cmd_list1 = lzt::create_command_list();
  cmdQueueVec cmd_queue;
  cmd_queue.resize(num_iterations);
  for (uint32_t i = 0; i < num_iterations; i++) {
    ze_command_queue_handle_t queue_handle = lzt::create_command_queue();
    cmd_queue[i] = queue_handle;
  }

  uint32_t group_size_x, group_size_y, group_size_z;
  lzt::suggest_group_size(scratch_function, array_size, 1, 1, group_size_x,
                          group_size_y, group_size_z);
  uint32_t groupSize = group_size_x * group_size_y * group_size_z;
  lzt::set_group_size(scratch_function, group_size_x, group_size_y,
                      group_size_z);
  lzt::set_argument_value(scratch_function, 0, sizeof(src_buffer), &src_buffer);
  lzt::set_argument_value(scratch_function, 1, sizeof(dst_buffer), &dst_buffer);
  lzt::set_argument_value(scratch_function, 2, sizeof(offset_buffer),
                          &offset_buffer);
  // if groupSize is greater then memory count, then at least one thread group
  // should be dispatched
  uint32_t threadGroup =
      array_size / groupSize > 1 ? array_size / groupSize : 1;
  ze_group_count_t thread_group_dimensions = {threadGroup, 1, 1};

  for (uint32_t i = 0; i < num_iterations; i++) {
    // Initialize memory
    memset(src_buffer, 0, in_memory_size);
    memset(dst_buffer, 0, out_memory_size);
    memset(offset_buffer, 0, offset_memory_size);
    memset(expected_memory, 0, expected_memory_size);

    auto src_buffer_uint = static_cast<uint32_t *>(src_buffer);
    auto expected_memory_uint = static_cast<uint32_t *>(expected_memory);
    const uint32_t in_value = 2u;
    const uint32_t expected_val1 =
        in_value * (array_size - 1u) * array_size / 2u;
    const uint32_t expected_val2 = in_value * in_value * array_size;

    for (uint32_t i = 0; i < array_size; ++i) {
      src_buffer_uint[i] = in_value;
      expected_memory_uint[i * 2] = expected_val1;
      expected_memory_uint[i * 2 + 1] = expected_val2;
    }

    memset(host_memory, 0, size);

    // dispatch cmd_list1 for memory fill/ copy
    lzt::append_memory_fill(cmd_list0, device_memory, &pattern, pattern_size,
                            size, nullptr);
    lzt::append_barrier(cmd_list0, nullptr, 0, nullptr);
    lzt::append_memory_copy(cmd_list0, host_memory, device_memory, size,
                            nullptr);
    lzt::append_barrier(cmd_list0, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_list0);
    lzt::execute_command_lists(cmd_queue[i], 1, &cmd_list0, nullptr);

    // dispatch cmd_list0 with kernel having scratch space
    lzt::append_launch_function(cmd_list1, scratch_function,
                                &thread_group_dimensions, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_list1);
    lzt::execute_command_lists(cmd_queue[i], 1, &cmd_list1, nullptr);

    lzt::synchronize(cmd_queue[i], UINT64_MAX);

    // Validate
    auto dst_buffer_uint = static_cast<uint32_t *>(dst_buffer);
    bool outputValidationSuccessful = true;
    if (memcmp(dst_buffer, expected_memory, expected_memory_size)) {
      outputValidationSuccessful = false;
      for (size_t i = 0; i < expected_memory_size / type_size; i++) {
        if (dst_buffer_uint[i] != expected_memory_uint[i]) {
          std::cout << "dstBuffer[" << i << "] = " << dst_buffer_uint[i]
                    << " not equal to "
                    << "expectedMemory[" << i
                    << "] = " << expected_memory_uint[i] << "\n";
          break;
        }
      }
    }

    for (uint32_t i = 0; i < size / sizeof(uint8_t); i++) {
      ASSERT_EQ(static_cast<uint8_t *>(host_memory)[i], pattern);
    }

    lzt::reset_command_list(cmd_list0);
    lzt::reset_command_list(cmd_list1);
  }

  lzt::destroy_command_list(cmd_list0);
  lzt::destroy_command_list(cmd_list1);
  for (uint32_t i = 0; i < num_iterations; i++) {
    lzt::destroy_command_queue(cmd_queue[i]);
  }
  cmd_queue.clear();
  lzt::free_memory_with_allocator_selector(dst_buffer,
                                           is_shared_system_scratch_kernel);
  lzt::free_memory_with_allocator_selector(src_buffer,
                                           is_shared_system_scratch_kernel);
  lzt::free_memory_with_allocator_selector(offset_buffer,
                                           is_shared_system_scratch_kernel);
  lzt::free_memory_with_allocator_selector(expected_memory,
                                           is_shared_system_scratch_kernel);
  lzt::free_memory_with_allocator_selector(device_memory, is_shared_system_src);
  lzt::free_memory_with_allocator_selector(host_memory, is_shared_system_dst);
  lzt::destroy_function(scratch_function);
  lzt::destroy_module(module_handle);
}

LZT_TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturned) {
  RunGivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturnedTest(
      false, false, false);
}

LZT_TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  for (int i = 1; i < 8; ++i) {
    std::bitset<3> bits(i);
    RunGivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturnedTest(
        bits[2], bits[1], bits[0]);
  }
}

class zeCommandListEventCounterTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    ze_event_pool_desc_t eventPoolDesc = {};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 10;

    ze_event_pool_counter_based_exp_desc_t counterBasedExtension = {
        ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedExtension.flags =
        ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_NON_IMMEDIATE;
    eventPoolDesc.pNext = &counterBasedExtension;
    ep.InitEventPool(eventPoolDesc);
    ep.create_event(event0, ZE_EVENT_SCOPE_FLAG_HOST, 0);

    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
    cmdlist.push_back(
        lzt::create_command_list(lzt::zeDevice::get_instance()->get_device(),
                                 ZE_COMMAND_LIST_FLAG_IN_ORDER));
    cmdqueue.push_back(lzt::create_command_queue(
        lzt::zeDevice::get_instance()->get_device(),
        ZE_COMMAND_QUEUE_FLAG_IN_ORDER, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0));
  }

  void TearDown() override {
    ep.destroy_event(event0);
    for (auto cl : cmdlist) {
      lzt::destroy_command_list(cl);
    }
    for (auto cq : cmdqueue) {
      lzt::destroy_command_queue(cq);
    }
  }
  cmdListVec cmdlist;
  cmdQueueVec cmdqueue;
  ze_event_handle_t event0 = nullptr;
};

static void RunAppendLaunchKernelEvent(cmdListVec cmdlist, cmdQueueVec cmdqueue,
                                       ze_event_handle_t event, int num_cmdlist,
                                       void *buffer, const size_t size) {

  const int addval = 10;
  const int num_iterations = 100;
  int addval2 = 0;
  const uint64_t timeout = UINT64_MAX - 1;

  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), "cmdlist_add.spv",
      ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel =
      lzt::create_function(module, "cmdlist_add_constant");
  lzt::set_group_size(kernel, 1, 1, 1);

  int totalVal[10];

  memset(buffer, 0x0, num_cmdlist * size * sizeof(int));

  for (int n = 0; n < num_cmdlist; n++) {
    int *p_dev = static_cast<int *>(buffer);
    p_dev += (n * size);
    lzt::set_argument_value(kernel, 0, sizeof(p_dev), &p_dev);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    ze_group_count_t tg;
    tg.groupCountX = static_cast<uint32_t>(size);
    tg.groupCountY = 1;
    tg.groupCountZ = 1;

    lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);

    totalVal[n] = 0;
    for (int i = 0; i < (num_iterations - 1); i++) {
      addval2 = lzt::generate_value<int>() & 0xFFFF;
      totalVal[n] += addval2;
      lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

      lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);
    }
    addval2 = lzt::generate_value<int>() & 0xFFFF;
    ;
    totalVal[n] += addval2;
    lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);
    if (n == 0) {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 0, nullptr);
    } else {
      lzt::append_launch_function(cmdlist[n], kernel, &tg, event, 0, &event);
    }
    lzt::close_command_list(cmdlist[n]);
    lzt::execute_command_lists(cmdqueue[n], 1, &cmdlist[n], nullptr);
    lzt::synchronize(cmdqueue[n], UINT64_MAX);
  }

  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, timeout));

  for (int n = 0; n < num_cmdlist; n++) {
    for (size_t i = 0; i < size; i++) {
      EXPECT_EQ(static_cast<int *>(buffer)[(n * size) + i],
                (addval + totalVal[n]));
    }
    lzt::reset_command_list(cmdlist[n]);
  }
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
}

static void RunAppendLaunchKernelEventL0SharedAlloc(cmdListVec cmdlist,
                                                    cmdQueueVec cmdqueue,
                                                    ze_event_handle_t event,
                                                    int num_cmdlist,
                                                    const size_t size) {
  void *buffer = lzt::allocate_shared_memory(num_cmdlist * size * sizeof(int));
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event, num_cmdlist, buffer,
                             size);
  lzt::free_memory(buffer);
}

static void RunAppendLaunchKernelEventHostMalloc(cmdListVec cmdlist,
                                                 cmdQueueVec cmdqueue,
                                                 ze_event_handle_t event,
                                                 int num_cmdlist,
                                                 const size_t size) {
  void *buffer = malloc(num_cmdlist * size * sizeof(int));
  ASSERT_NE(nullptr, buffer);
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event, num_cmdlist, buffer,
                             size);
  free(buffer);
}

static void RunAppendLaunchKernelEventHostNew(cmdListVec cmdlist,
                                              cmdQueueVec cmdqueue,
                                              ze_event_handle_t event,
                                              int num_cmdlist,
                                              const size_t size) {
  int *buffer = new int[num_cmdlist * size];
  ASSERT_NE(nullptr, buffer);
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event, num_cmdlist,
                             reinterpret_cast<void *>(buffer), size);
  delete[] buffer;
}

static void RunAppendLaunchKernelEventHostUniquePtr(cmdListVec cmdlist,
                                                    cmdQueueVec cmdqueue,
                                                    ze_event_handle_t event,
                                                    int num_cmdlist,
                                                    const size_t size) {
  std::unique_ptr<int[]> buffer(new int[num_cmdlist * size]);
  ASSERT_NE(nullptr, buffer);
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event, num_cmdlist,
                             reinterpret_cast<void *>(buffer.get()), size);
}

static void RunAppendLaunchKernelEventHostSharedPtr(cmdListVec cmdlist,
                                                    cmdQueueVec cmdqueue,
                                                    ze_event_handle_t event,
                                                    int num_cmdlist,
                                                    const size_t size) {
  std::shared_ptr<int[]> buffer(new int[num_cmdlist * size]);
  ASSERT_NE(nullptr, buffer);
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event, num_cmdlist,
                             reinterpret_cast<void *>(buffer.get()), size);
}

typedef void (*RunAppendLaunchKernelEventFunc)(cmdListVec, cmdQueueVec,
                                               ze_event_handle_t, int,
                                               const size_t);

static void
RunAppendLaunchKernelEventLoop(cmdListVec cmdlist, cmdQueueVec cmdqueue,
                               ze_event_handle_t event,
                               RunAppendLaunchKernelEventFunc func) {
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(), ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME)) {
    GTEST_SKIP() << "Extension " << ZE_EVENT_POOL_COUNTER_BASED_EXP_NAME
                 << " not supported";
  }

  constexpr size_t size = 16;
  for (int i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Testing " << i << " command list(s)";
    func(cmdlist, cmdqueue, event, i, size);
  }
}

LZT_TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution) {
  RunAppendLaunchKernelEventLoop(cmdlist, cmdqueue, event0,
                                 RunAppendLaunchKernelEventL0SharedAlloc);
}

LZT_TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionUsingMallocWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendLaunchKernelEventLoop(cmdlist, cmdqueue, event0,
                                 RunAppendLaunchKernelEventHostMalloc);
}

LZT_TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionUsingNewWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendLaunchKernelEventLoop(cmdlist, cmdqueue, event0,
                                 RunAppendLaunchKernelEventHostNew);
}

LZT_TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionUsingUniquePtrWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendLaunchKernelEventLoop(cmdlist, cmdqueue, event0,
                                 RunAppendLaunchKernelEventHostUniquePtr);
}

LZT_TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionUsingSharedPtrWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunAppendLaunchKernelEventLoop(cmdlist, cmdqueue, event0,
                                 RunAppendLaunchKernelEventHostSharedPtr);
}

} // namespace
