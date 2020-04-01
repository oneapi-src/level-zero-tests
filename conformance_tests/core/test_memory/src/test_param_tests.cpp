/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
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

namespace {

enum memory_test_type { MTT_SHARED, MTT_HOST };

class zeMemAccessTests : public ::testing::Test,
                         public ::testing::WithParamInterface<
                             std::tuple<enum memory_test_type, size_t>> {

protected:
  zeMemAccessTests() {
    memory_ = nullptr;
    mtt_ = std::get<0>(GetParam());
    const size_t alignment = std::get<1>(GetParam());
    if (mtt_ == MTT_HOST)
      memory_ = lzt::allocate_host_memory(size_, alignment);
    else
      memory_ = lzt::allocate_shared_memory(size_, alignment);
  }
  ~zeMemAccessTests() { lzt::free_memory(memory_); }

  const size_t size_ = 4 * 1024;
  void *memory_ = nullptr;
  memory_test_type mtt_;
};

TEST_P(zeMemAccessTests,
       GivenMemAllocationWhenWritingAndReadingBackOnHostThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  lzt::validate_data_pattern(memory_, size_, 1);
}

class zeMemAccessCommandListTests : public zeMemAccessTests {
protected:
  lzt::zeCommandList cmdlist_;
  lzt::zeCommandQueue cmdqueue_;
};

TEST_P(
    zeMemAccessCommandListTests,
    GivenMemoryAllocationWhenCopyingAndReadingBackOnHostThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  const size_t alignment = std::get<1>(GetParam());
  void *other_memory = (mtt_ == MTT_HOST)
                           ? lzt::allocate_host_memory(size_, alignment)
                           : lzt::allocate_shared_memory(size_, alignment);

  lzt::append_memory_copy(cmdlist_.command_list_, other_memory, memory_, size_,
                          nullptr);
  lzt::append_barrier(cmdlist_.command_list_, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist_.command_list_);
  lzt::execute_command_lists(cmdqueue_.command_queue_, 1,
                             &cmdlist_.command_list_, nullptr);
  lzt::synchronize(cmdqueue_.command_queue_, UINT32_MAX);
  lzt::validate_data_pattern(other_memory, size_, 1);
  lzt::free_memory(other_memory);
}

TEST_P(zeMemAccessCommandListTests,
       GivenAllocationSettingAndReadingBackOnHostThenCorrectDataIsRead) {
  const uint8_t value = 0x55;
  memset(memory_, 0,
         size_); // Write a different pattern from what we are going to write.
  lzt::append_memory_set(cmdlist_.command_list_, memory_, &value, size_);
  lzt::append_barrier(cmdlist_.command_list_, nullptr, 0, nullptr);
  lzt::close_command_list(cmdlist_.command_list_);
  lzt::execute_command_lists(cmdqueue_.command_queue_, 1,
                             &cmdlist_.command_list_, nullptr);
  lzt::synchronize(cmdqueue_.command_queue_, UINT32_MAX);
  for (unsigned int ui = 0; ui < size_; ui++)
    EXPECT_EQ(value, static_cast<uint8_t *>(memory_)[ui]);
}

TEST_P(zeMemAccessTests,
       GivenAllocationWhenWritingAndReadingBackOnDeviceThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  std::string module_name = "unified_mem_test.spv";
  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), module_name);
  std::string func_name = "unified_mem_test";

  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  arg.arg_size = sizeof(void *);
  arg.arg_value = &memory_;
  args.push_back(arg);
  arg.arg_size = sizeof(int);
  int size = static_cast<int>(size_);
  arg.arg_value = &size;
  args.push_back(arg);
  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name, 1, args);
  lzt::validate_data_pattern(memory_, size_, -1);
  lzt::destroy_module(module);
}

INSTANTIATE_TEST_CASE_P(zeMemAccessTests, zeMemAccessTests,
                        ::testing::Combine(testing::Values(MTT_HOST,
                                                           MTT_SHARED),
                                           lzt::memory_allocation_alignments));
INSTANTIATE_TEST_CASE_P(zeMemAccessCommandListTests,
                        zeMemAccessCommandListTests,
                        ::testing::Combine(testing::Values(MTT_HOST,
                                                           MTT_SHARED),
                                           lzt::memory_allocation_alignments));
INSTANTIATE_TEST_CASE_P(zeMemAccessCommandListTests_1,
                        zeMemAccessCommandListTests,
                        ::testing::Combine(testing::Values(MTT_HOST,
                                                           MTT_SHARED),
                                           lzt::memory_allocation_alignments));
INSTANTIATE_TEST_CASE_P(zeMemAccessDeviceTests, zeMemAccessTests,
                        ::testing::Combine(testing::Values(MTT_HOST,
                                                           MTT_SHARED),
                                           lzt::memory_allocation_alignments));

} // namespace
