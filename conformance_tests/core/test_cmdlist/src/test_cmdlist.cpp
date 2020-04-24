/*
 *
 * Copyright (C) 2019 Intel Corporation
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

class zeCommandListCreateTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_list_flag_t> {};

TEST_P(
    zeCommandListCreateTests,
    GivenValidDeviceAndCommandListDescriptorWhenCreatingCommandListThenNotNullCommandListIsReturned) {
  ze_command_list_desc_t descriptor = {};
  descriptor.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;
  descriptor.flags = GetParam();

  if (descriptor.flags == ZE_COMMAND_LIST_FLAG_COPY_ONLY) {
    auto properties =
        lzt::get_device_properties(lzt::zeDevice::get_instance()->get_device());
    if (properties.numAsyncCopyEngines == 0) {
      LOG_WARNING << "Not Enough Copy Engines to run test";
      SUCCEED();
      return;
    }
  }
  ze_command_list_handle_t command_list = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListCreate(lzt::zeDevice::get_instance()->get_device(),
                                &descriptor, &command_list));
  EXPECT_NE(nullptr, command_list);

  lzt::destroy_command_list(command_list);
}

INSTANTIATE_TEST_CASE_P(
    CreateFlagParameterizedTest, zeCommandListCreateTests,
    ::testing::Values(ZE_COMMAND_LIST_FLAG_NONE, ZE_COMMAND_LIST_FLAG_COPY_ONLY,
                      ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING));

class zeCommandListDestroyTests : public ::testing::Test {};

TEST_F(
    zeCommandListDestroyTests,
    GivenValidDeviceAndCommandListDescriptorWhenDestroyingCommandListThenSuccessIsReturned) {
  ze_command_list_desc_t descriptor = {};
  descriptor.version = ZE_COMMAND_LIST_DESC_VERSION_CURRENT;

  ze_command_list_handle_t command_list = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListCreate(lzt::zeDevice::get_instance()->get_device(),
                                &descriptor, &command_list));
  EXPECT_NE(nullptr, command_list);

  lzt::destroy_command_list(command_list);
}

class zeCommandListCreateImmediateTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_command_queue_flag_t, ze_command_queue_mode_t,
                     ze_command_queue_priority_t>> {};

TEST_P(zeCommandListCreateImmediateTests,
       GivenImplicitCommandQueueWhenCreatingCommandListThenSuccessIsReturned) {

  ze_command_queue_desc_t descriptor = {
      ZE_COMMAND_QUEUE_DESC_VERSION_CURRENT, // version
      std::get<0>(GetParam()),               // flags
      std::get<1>(GetParam()),               // mode
      std::get<2>(GetParam())                // priority
  };

  if (descriptor.flags == ZE_COMMAND_LIST_FLAG_COPY_ONLY) {
    auto properties =
        lzt::get_device_properties(lzt::zeDevice::get_instance()->get_device());
    if (properties.numAsyncCopyEngines == 0) {
      LOG_WARNING << "Not Enough Copy Engines to run test";
      SUCCEED();
      return;
    }
  }
  ze_command_list_handle_t command_list = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListCreateImmediate(
                                   lzt::zeDevice::get_instance()->get_device(),
                                   &descriptor, &command_list));

  lzt::destroy_command_list(command_list);
}

INSTANTIATE_TEST_CASE_P(
    ImplictCommandQueueParameterizedTest, zeCommandListCreateImmediateTests,
    ::testing::Combine(
        ::testing::Values(ZE_COMMAND_QUEUE_FLAG_NONE,
                          ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
                          ZE_COMMAND_QUEUE_FLAG_LOGICAL_ONLY,
                          ZE_COMMAND_QUEUE_FLAG_SINGLE_SLICE_ONLY),
        ::testing::Values(ZE_COMMAND_QUEUE_MODE_DEFAULT,
                          ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
                          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS),
        ::testing::Values(ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
                          ZE_COMMAND_QUEUE_PRIORITY_LOW,
                          ZE_COMMAND_QUEUE_PRIORITY_HIGH)));

class zeCommandListCloseTests : public lzt::zeCommandListTests {};

TEST_F(zeCommandListCloseTests,
       GivenEmptyCommandListWhenClosingCommandListThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(cl.command_list_));
}

class zeCommandListResetTests : public ::testing::Test {
protected:
  void run_reset_test(bool execute_all_commands) {
    auto device = lzt::zeDevice::get_instance()->get_device();
    auto command_list = lzt::create_command_list();
    auto command_queue = lzt::create_command_queue();

    const size_t size = 16;
    std::vector<uint8_t> host_memory1(size);
    std::vector<uint8_t> host_memory2(size);
    auto device_dest = lzt::allocate_device_memory(size);
    auto device_src = lzt::allocate_device_memory(size);
    auto host_mem = lzt::allocate_host_memory(size);

    // Append various operations to the command list
    lzt::write_data_pattern(host_memory1.data(), size, 0);
    lzt::append_memory_copy(command_list, device_dest, host_memory1.data(),
                            size);
    lzt::append_barrier(command_list);
    lzt::write_data_pattern(host_memory2.data(), size, 1);
    lzt::append_memory_copy(command_list, device_src, host_memory2.data(),
                            size);
    lzt::append_barrier(command_list);
    lzt::write_data_pattern(host_mem, size, -1);
    lzt::append_memory_copy(command_list, device_dest, device_src, size);
    lzt::append_barrier(command_list);
    lzt::append_memory_copy(command_list, host_mem, device_dest, size);
    lzt::append_barrier(command_list);

    uint8_t pattern = 0xAA;
    int pattern_size = 1;
    auto memory_fill_mem = lzt::allocate_shared_memory(size);
    memset(memory_fill_mem, 0, size);
    lzt::append_memory_fill(command_list, memory_fill_mem, &pattern,
                            pattern_size, size, nullptr);
    lzt::append_barrier(command_list);

    lzt::zeImageCreateCommon img;
    lzt::ImagePNG32Bit dest_host_image(img.dflt_host_image_.width(),
                                       img.dflt_host_image_.height());
    lzt::write_image_data_pattern(dest_host_image, -1);
    lzt::append_image_copy_from_mem(command_list, img.dflt_device_image_,
                                    img.dflt_host_image_.raw_data(), nullptr);
    lzt::append_barrier(command_list);
    lzt::append_image_copy_to_mem(command_list, dest_host_image.raw_data(),
                                  img.dflt_device_image_, nullptr);

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
    lzt::append_launch_function(command_list, kernel, &args, nullptr, 0,
                                nullptr);

    ze_event_handle_t event = nullptr;
    ze_event_pool_desc_t event_pool_desc = {ZE_EVENT_POOL_DESC_VERSION_CURRENT,
                                            ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
    ze_event_desc_t event_desc = {ZE_EVENT_DESC_VERSION_CURRENT, 0,
                                  ZE_EVENT_SCOPE_FLAG_HOST,
                                  ZE_EVENT_SCOPE_FLAG_HOST};
    lzt::zeEventPool ep;
    ep.InitEventPool(event_pool_desc);
    ep.create_event(event, event_desc);
    lzt::append_signal_event(command_list, event);

    lzt::close_command_list(command_list);
    if (execute_all_commands) {
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      lzt::synchronize(command_queue, UINT32_MAX);
    }

    lzt::reset_command_list(command_list);

    auto test_mem = lzt::allocate_shared_memory(size);
    memset(test_mem, 0, size);
    lzt::append_memory_fill(command_list, test_mem, &pattern, sizeof(uint8_t),
                            size, nullptr);
    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT32_MAX);

    if (execute_all_commands) {
      lzt::validate_data_pattern(host_mem, size, 1);

      EXPECT_EQ(0, compare_data_pattern(dest_host_image, img.dflt_host_image_,
                                        0, 0, img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height(), 0, 0,
                                        img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height()));

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

      EXPECT_NE(0, compare_data_pattern(dest_host_image, img.dflt_host_image_,
                                        0, 0, img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height(), 0, 0,
                                        img.dflt_host_image_.width(),
                                        img.dflt_host_image_.height()));

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
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
};

TEST_F(zeCommandListResetTests,
       GivenEmptyCommandListWhenResettingCommandListThenSuccessIsReturned) {
  auto command_list = lzt::create_command_list();
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(command_list));
  lzt::destroy_command_list(command_list);
}

TEST_F(
    zeCommandListResetTests,
    GivenResetCommandListWithVariousCommandsWhenExecutingMemoryFillThenOnlyMemoryFillExecuted) {
  run_reset_test(false);
}
TEST_F(
    zeCommandListResetTests,
    GivenResetExecutedCommandListWithVariousCommandsWhenExecutingMemoryFillThenAllCommandsExecuted) {
  run_reset_test(true);
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
    lzt::synchronize(cmdq, UINT32_MAX);
    for (int j = 0; j < size; j++)
      ASSERT_EQ(static_cast<uint8_t *>(host_buffer)[j], 0x0)
          << "Memory Set did not match.";

    lzt::execute_command_lists(cmdq, 1, &cmdlist_mem_set, nullptr);
    lzt::synchronize(cmdq, UINT32_MAX);
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

class zeCommandListCloseAndResetTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_list_flag_t> {};

TEST_P(
    zeCommandListCloseAndResetTests,
    DISABLED_GivenResetCommandListWhenCloseImmediatelyNoCommandListInstructionsExecute) {

  ze_command_list_flag_t flags = GetParam();
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_list_handle_t cmdlist = lzt::create_command_list(device, flags);
  ze_command_queue_handle_t cmdq;
  if (flags == ZE_COMMAND_LIST_FLAG_COPY_ONLY) {
    cmdq = lzt::create_command_queue(device, ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
                                     ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                     ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  } else {
    cmdq = lzt::create_command_queue(device);
  }
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
  lzt::append_memory_set(cmdlist, buffer, &set_succeed_1, size);
  lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist);
  // Attempt to append command list after close should fail
  lzt::append_memory_set(cmdlist, buffer, &set_fail_2, size);
  lzt::execute_command_lists(cmdq, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdq, UINT32_MAX);
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], set_succeed_1);
  }

  memset(buffer, 0x0, size);
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], 0x0);
  }
  // Reset command list and immediately close
  lzt::reset_command_list(cmdlist);
  lzt::close_command_list(cmdlist);
  // Attempt to append command list after close should fail
  lzt::append_memory_set(cmdlist, buffer, &set_fail_3, size);
  lzt::execute_command_lists(cmdq, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdq, UINT32_MAX);
  // No commands should be executed by command queue
  for (size_t j = 0; j < size; j++) {
    EXPECT_EQ(static_cast<uint8_t *>(buffer)[j], 0x0);
  }
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdq);
  lzt::free_memory(buffer);
}

TEST_P(zeCommandListCloseAndResetTests,
       GivenCommandListWhenResetThenVerifyOnlySubsequentInstructionsExecuted) {
  ze_command_list_flag_t flags = GetParam();
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_command_queue_handle_t cmdq;
  if (flags == ZE_COMMAND_LIST_FLAG_COPY_ONLY) {
    auto properties = lzt::get_device_properties(device);
    if (properties.numAsyncCopyEngines == 0) {
      LOG_WARNING << "Not Enough Copy Engines to run test";
      SUCCEED();
      return;
    }
    cmdq = lzt::create_command_queue(device, ZE_COMMAND_QUEUE_FLAG_COPY_ONLY,
                                     ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                     ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  } else {
    cmdq = lzt::create_command_queue(device);
  }
  ze_command_list_handle_t cmdlist = lzt::create_command_list(device, flags);
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
      lzt::append_memory_set(cmdlist, buffer[i], &val[test_instr - (i + 1)],
                             size);
    }
    lzt::append_barrier(cmdlist, nullptr, 0, nullptr);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdq, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdq, UINT32_MAX);
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
    lzt::reset_command_list(cmdlist);
    test_instr--;
  }
  // Last check:  no instructions should be executed
  for (auto buf : buffer) {
    memset(buf, 0x0, size);
  }
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdq, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdq, UINT32_MAX);
  for (size_t i = 0; i < num_instr; i++) {
    for (size_t j = 0; j < size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(buffer[i])[j], 0x0);
    }
  }

  // Command list setup with only memory set and barrier

  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdq);
  for (auto buf : buffer) {
    lzt::free_memory(buf);
  }
}

INSTANTIATE_TEST_CASE_P(
    TestCasesforCommandListCloseAndCommandListReset,
    zeCommandListCloseAndResetTests,
    testing::Values(ZE_COMMAND_LIST_FLAG_NONE, ZE_COMMAND_LIST_FLAG_COPY_ONLY,
                    ZE_COMMAND_LIST_FLAG_RELAXED_ORDERING,
                    ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                    ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY));
class zeCommandListFlagTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_command_list_flag_t> {};
TEST_P(zeCommandListFlagTests,
       GivenCommandListCreatedWithDifferentFlagsThenSuccessIsReturned) {
  size_t size = 1024;
  auto device_memory = lzt::allocate_device_memory(size);
  auto host_memory = lzt::allocate_host_memory(size);
  auto device = lzt::zeDevice::get_instance()->get_device();
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;
  ze_command_list_flag_t flag = GetParam();
  auto cq = lzt::create_command_queue();
  auto command_list = lzt::create_command_list(device, flag);

  lzt::append_memory_fill(command_list, device_memory, &pattern, pattern_size,
                          size, nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  lzt::append_memory_copy(command_list, host_memory, device_memory, size,
                          nullptr);
  lzt::append_barrier(command_list, nullptr, 0, nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(cq, 1, &command_list, nullptr);
  lzt::synchronize(cq, UINT32_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(host_memory)[i], pattern);
  }
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(cq);
  lzt::free_memory(host_memory);
  lzt::free_memory(device_memory);
}
INSTANTIATE_TEST_CASE_P(
    CreateFlagParameterizedTest, zeCommandListFlagTests,
    ::testing::Values(ZE_COMMAND_LIST_FLAG_NONE,
                      ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT,
                      ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY));

} // namespace
