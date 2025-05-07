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
#include <chrono>
namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeCommandListCreateTests : public ::testing::Test,
                                 public ::testing::WithParamInterface<
                                     std::tuple<ze_command_list_flag_t, bool>> {
};

TEST_P(
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

TEST_F(
    zeCommandListDestroyTests,
    GivenValidDeviceAndCommandListDescriptorWhenDestroyingCommandListThenSuccessIsReturned) {
  run(false);
}

TEST_F(
    zeCommandListDestroyTests,
    GivenValidDeviceAndImmediateCommandListDescriptorWhenDestroyingCommandListThenSuccessIsReturned) {
  run(true);
}

class zeCommandListCreateImmediateTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flag_t, ze_command_queue_mode_t,
                     ze_command_queue_priority_t>> {};

TEST_P(zeCommandListCreateImmediateTests,
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
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(bundle.list));
    lzt::destroy_command_bundle(bundle);
  }
};

TEST_F(zeCommandListCloseTests,
       GivenEmptyCommandListWhenClosingCommandListThenSuccessIsReturned) {
  run(false);
}

TEST_F(
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
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event));
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

TEST_F(zeCommandListResetTests,
       GivenEmptyCommandListWhenResettingCommandListThenSuccessIsReturned) {
  auto bundle = lzt::create_command_bundle(false);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(bundle.list));
  lzt::destroy_command_bundle(bundle);
}

TEST_F(
    zeCommandListResetTests,
    GivenEmptyImmediateCommandListWhenResettingCommandListThenSuccessIsReturned) {
  auto bundle = lzt::create_command_bundle(true);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(bundle.list));
  lzt::destroy_command_bundle(bundle);
}

TEST_F(
    zeCommandListResetTests,
    GivenResetCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenOnlyMemoryFillExecuted) {
  run_reset_test(false, false);
}

TEST_F(
    zeCommandListResetTests,
    GivenResetImmediateCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenOnlyMemoryFillExecuted) {
  run_reset_test(false, true);
}

TEST_F(
    zeCommandListResetTests,
    GivenResetExecutedCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenAllCommandsExecuted) {
  run_reset_test(true, false);
}

TEST_F(
    zeCommandListResetTests,
    GivenResetExecutedImmediateCommandListWithVariousCommandsIncludingImageCommandsWhenExecutingMemoryFillThenAllCommandsExecuted) {
  run_reset_test(true, true);
}

class zeCommandListReuseTests : public ::testing::Test {};

TEST(zeCommandListReuseTests, GivenCommandListWhenItIsExecutedItCanBeRunAgain) {
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

TEST_P(
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

TEST_P(zeCommandListCloseAndResetTests,
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
TEST_P(zeCommandListFlagTests,
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

TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopThenSuccessIsReturned) {
  auto cq = lzt::create_command_queue();
  auto cmd_list = lzt::create_command_list();
  const int transfer_size = 1;
  const int num_iteration = 3;
  const uint8_t max_copy_count = 100;
  const size_t copy_size = 100;
  auto host_memory_src = lzt::allocate_host_memory(copy_size);
  auto host_memory_dst = lzt::allocate_host_memory(copy_size);

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
  lzt::free_memory(host_memory_src);
  lzt::free_memory(host_memory_dst);
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
  ze_event_handle_t wait_event_before, wait_event_after;
  auto wait_event_initial = event;

  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendWriteGlobalTimestamp(bundle.list, &dst_timestamp,
                                                    nullptr, 1, &event));
  ASSERT_EQ(event, wait_event_initial);

  lzt::close_command_list(bundle.list);
  lzt::signal_event_from_host(event);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

  ep.destroy_event(event);
  lzt::destroy_command_bundle(bundle);
}

TEST(zeCommandListAppendWriteGlobalTimestampTest,
     GivenCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned) {
  RunAppendingWriteGlobalTimestampThenSuccessIsReturned(false);
}

TEST(
    zeCommandListAppendWriteGlobalTimestampTest,
    GivenImmediateCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned) {
  RunAppendingWriteGlobalTimestampThenSuccessIsReturned(true);
}

TEST(
    zeCommandListAppendMemoryCopyTest,
    GivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturned) {
  // Create buffers for scratch kernel
  uint32_t arraySize = 32;
  uint32_t typeSize = sizeof(uint32_t);
  uint32_t expectedMemorySize = (arraySize * 2 + 1) * typeSize - 4;
  uint32_t inMemorySize = expectedMemorySize;
  uint32_t outMemorySize = expectedMemorySize;
  uint32_t offsetMemorySize = 128 * arraySize;

  ze_device_mem_alloc_desc_t deviceDesc = {};
  deviceDesc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  deviceDesc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
  deviceDesc.ordinal = 0;
  ze_host_mem_alloc_desc_t hostDesc = {};
  hostDesc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  hostDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
  void *srcBuffer = lzt::allocate_host_memory(inMemorySize);
  void *dstBuffer = lzt::allocate_host_memory(outMemorySize);
  void *offsetBuffer = lzt::allocate_host_memory(offsetMemorySize);
  void *expectedMemory = lzt::allocate_host_memory(expectedMemorySize);

  // create two buffers for append fill/ copy kernel
  size_t size = 1024;
  auto device_memory = lzt::allocate_device_memory(size);
  auto host_memory = lzt::allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  uint32_t num_iterations = 2;

  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  ze_module_handle_t module_handle =
      lzt::create_module(device, "cmdlist_scratch.spv");
  ze_kernel_flags_t flag = 0;
  /* Prepare the fill function */
  ze_kernel_handle_t scratch_function =
      lzt::create_function(module_handle, flag, "spill_test");
  ze_kernel_properties_t kernelProperties = {};
  kernelProperties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  zeKernelGetProperties(scratch_function, &kernelProperties);
  std::cout << "Scratch size = " << kernelProperties.spillMemSize << "\n";
  if (kernelProperties.spillMemSize == 0) {
    lzt::destroy_function(scratch_function);
    lzt::destroy_module(module_handle);
    GTEST_SKIP() << "Kernel does not use scratch space\n";
  }

  auto cmd_list0 = lzt::create_command_list();
  auto cmd_list1 = lzt::create_command_list();
  std::vector<ze_command_queue_handle_t> cmd_queue;
  cmd_queue.resize(num_iterations);
  for (uint32_t i = 0; i < num_iterations; i++) {
    ze_command_queue_handle_t queue_handle = lzt::create_command_queue();
    cmd_queue[i] = queue_handle;
  }

  uint32_t groupSizeX, groupSizeY, groupSizeZ;
  lzt::suggest_group_size(scratch_function, arraySize, 1, 1, groupSizeX,
                          groupSizeY, groupSizeZ);
  size_t groupSize = groupSizeX * groupSizeY * groupSizeZ;
  lzt::set_group_size(scratch_function, groupSizeX, groupSizeY, groupSizeZ);
  lzt::set_argument_value(scratch_function, 2, sizeof(dstBuffer), &dstBuffer);
  lzt::set_argument_value(scratch_function, 1, sizeof(srcBuffer), &srcBuffer);
  lzt::set_argument_value(scratch_function, 0, sizeof(offsetBuffer),
                          &offsetBuffer);
  // if groupSize is greater then memory count, then at least one thread group
  // should be dispatched
  uint32_t threadGroup = arraySize / groupSize > 1 ? arraySize / groupSize : 1;
  ze_group_count_t thread_group_dimensions = {threadGroup, 1, 1};

  for (uint32_t i = 0; i < num_iterations; i++) {
    // Initialize memory
    memset(srcBuffer, 0, inMemorySize);
    memset(dstBuffer, 0, outMemorySize);
    memset(offsetBuffer, 0, offsetMemorySize);
    memset(expectedMemory, 0, expectedMemorySize);

    auto srcBufferInt = static_cast<uint32_t *>(srcBuffer);
    auto expectedMemoryInt = static_cast<uint32_t *>(expectedMemory);
    constexpr int expectedVal1 = 16256;
    constexpr int expectedVal2 = 512;

    for (uint32_t i = 0; i < arraySize; ++i) {
      srcBufferInt[i] = 2;
      expectedMemoryInt[i * 2] = expectedVal1;
      expectedMemoryInt[i * 2 + 1] = expectedVal2;
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

    bool outputValidationSuccessful = true;
    if (memcmp(dstBuffer, expectedMemory, expectedMemorySize)) {
      outputValidationSuccessful = false;
      uint8_t *srcCharBuffer = static_cast<uint8_t *>(expectedMemory);
      uint8_t *dstCharBuffer = static_cast<uint8_t *>(dstBuffer);
      for (size_t i = 0; i < expectedMemorySize; i++) {
        if (srcCharBuffer[i] != dstCharBuffer[i]) {
          std::cout << "srcBuffer[" << i
                    << "] = " << static_cast<unsigned int>(srcCharBuffer[i])
                    << " not equal to "
                    << "dstBuffer[" << i
                    << "] = " << static_cast<unsigned int>(dstCharBuffer[i])
                    << "\n";
          break;
        }
      }
    }

    for (uint32_t i = 0; i < size; i++) {
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
  lzt::free_memory(host_memory);
  lzt::free_memory(device_memory);
  lzt::free_memory(dstBuffer);
  lzt::free_memory(srcBuffer);
  lzt::free_memory(offsetBuffer);
  lzt::free_memory(expectedMemory);
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
  std::vector<ze_command_list_handle_t> cmdlist;
  std::vector<ze_command_queue_handle_t> cmdqueue;
  ze_event_handle_t event0 = nullptr;
};

static void
RunAppendLaunchKernelEvent(std::vector<ze_command_list_handle_t> cmdlist,
                           std::vector<ze_command_queue_handle_t> cmdqueue,
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
      addval2 = rand() & 0xFFFF;
      totalVal[n] += addval2;
      lzt::set_argument_value(kernel, 1, sizeof(addval2), &addval2);

      lzt::append_launch_function(cmdlist[n], kernel, &tg, nullptr, 0, nullptr);
    }
    addval2 = rand() & 0xFFFF;
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

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventHostSynchronize(event, timeout));

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

TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution) {

  bool event_pool_ext_found = lzt::check_if_extension_supported(
      lzt::get_default_driver(), "ZE_experimental_event_pool_counter_based");
  EXPECT_TRUE(event_pool_ext_found);

  for (int i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Testing " << i << " command list(s)";
    const size_t size = 16;
    void *buffer = lzt::allocate_shared_memory(i * size * sizeof(int));
    RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event0, i, buffer, size);
    lzt::free_memory(buffer);
  }
}

TEST_F(
    zeCommandListEventCounterTests,
    GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionWithSharedSystemAllocator) {

  bool event_pool_ext_found = lzt::check_if_extension_supported(
      lzt::get_default_driver(), "ZE_experimental_event_pool_counter_based");
  EXPECT_TRUE(event_pool_ext_found);

  lzt::gtest_skip_if_shared_system_alloc_unsupported(true);

  for (int i = 1; i <= cmdlist.size(); i++) {
    LOG_INFO << "Testing " << i << " command list(s)";
    const size_t size = 16;
    void *buffer = malloc(i * size * sizeof(int));
    ASSERT_NE(nullptr, buffer);
    RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event0, i, buffer, size);
    free(buffer);
  }
}

TEST_F(
  zeCommandListEventCounterTests,
  GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecutionWithSharedSystemAllocatorNew) {

bool event_pool_ext_found = lzt::check_if_extension_supported(
    lzt::get_default_driver(), "ZE_experimental_event_pool_counter_based");
EXPECT_TRUE(event_pool_ext_found);

lzt::gtest_skip_if_shared_system_alloc_unsupported(true);

for (int i = 1; i <= cmdlist.size(); i++) {
  LOG_INFO << "Testing " << i << " command list(s)";
  const size_t size = 16;
  void *buffer = reinterpret_cast<void *>(new int[i * size]);
  ASSERT_NE(nullptr, buffer);
  RunAppendLaunchKernelEvent(cmdlist, cmdqueue, event0, i, buffer, size);
  delete [] buffer;
}
}

} // namespace
