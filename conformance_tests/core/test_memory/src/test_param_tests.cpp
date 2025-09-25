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

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

using lzt::to_int;

enum memory_test_type { MTT_SHARED, MTT_HOST };

class zeMemAccessTests : public ::testing::Test,
                         public ::testing::WithParamInterface<
                             std::tuple<enum memory_test_type, size_t, bool>> {

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

LZT_TEST_P(
    zeMemAccessTests,
    GivenMemAllocationWhenWritingAndReadingBackOnHostThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  lzt::validate_data_pattern(memory_, size_, 1);
}

class zeMemAccessCommandListTests : public zeMemAccessTests {
protected:
  lzt::zeCommandBundle cmd_bundle_;
};

LZT_TEST_P(
    zeMemAccessCommandListTests,
    GivenMemoryAllocationWhenCopyingAndReadingBackOnHostThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  const size_t alignment = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());
  cmd_bundle_ = lzt::create_command_bundle(is_immediate);
  void *other_memory = (mtt_ == MTT_HOST)
                           ? lzt::allocate_host_memory(size_, alignment)
                           : lzt::allocate_shared_memory(size_, alignment);

  lzt::append_memory_copy(cmd_bundle_.list, other_memory, memory_, size_,
                          nullptr);
  lzt::append_barrier(cmd_bundle_.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle_.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_, UINT64_MAX);
  lzt::validate_data_pattern(other_memory, size_, 1);
  lzt::free_memory(other_memory);
  lzt::destroy_command_bundle(cmd_bundle_);
}

LZT_TEST_P(zeMemAccessCommandListTests,
           GivenAllocationSettingAndReadingBackOnHostThenCorrectDataIsRead) {
  bool is_immediate = std::get<2>(GetParam());
  cmd_bundle_ = lzt::create_command_bundle(is_immediate);
  const uint8_t value = 0x55;
  memset(memory_, 0,
         size_); // Write a different pattern from what we are going to write.
  lzt::append_memory_set(cmd_bundle_.list, memory_, &value, size_);
  lzt::append_barrier(cmd_bundle_.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle_.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_, UINT64_MAX);
  for (unsigned int ui = 0; ui < size_; ui++) {
    EXPECT_EQ(value, static_cast<uint8_t *>(memory_)[ui]);
  }
  lzt::destroy_command_bundle(cmd_bundle_);
}

LZT_TEST_P(
    zeMemAccessTests,
    GivenAllocationWhenWritingAndReadingBackOnDeviceThenCorrectDataIsRead) {
  bool is_immediate = std::get<2>(GetParam());
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
  int size = to_int(size_);
  arg.arg_value = &size;
  args.push_back(arg);
  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name, 1U, args, is_immediate);
  lzt::validate_data_pattern(memory_, size_, -1);
  lzt::destroy_module(module);
}

INSTANTIATE_TEST_SUITE_P(
    zeMemAccessTests, zeMemAccessTests,
    ::testing::Combine(::testing::Values(MTT_HOST, MTT_SHARED),
                       lzt::memory_allocation_alignments, ::testing::Bool()));
INSTANTIATE_TEST_SUITE_P(
    zeMemAccessCommandListTests, zeMemAccessCommandListTests,
    ::testing::Combine(::testing::Values(MTT_HOST, MTT_SHARED),
                       lzt::memory_allocation_alignments, ::testing::Bool()));

} // namespace
