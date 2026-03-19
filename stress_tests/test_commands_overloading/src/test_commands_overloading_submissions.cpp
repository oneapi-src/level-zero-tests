/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "stress_common_func.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

LZT_TEST(
    zeCommandListSubmissionsStressTest,
    GivenUint32TypeMaxCommandListSubmissionsOnImmediateCommandListWhenExecutingThenSuccessIsReturned) {
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device =
      lzt::get_default_device(lzt::get_default_driver());

  const uint64_t submission_count = std::numeric_limits<uint32_t>::max();

  ze_command_list_handle_t cmd_list = lzt::create_immediate_command_list(
      device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  ;
  ze_module_handle_t module_handle =
      lzt::create_module(context, device, "test_commands_overloading.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t kernel_handle =
      lzt::create_function(module_handle, "test_submissions");
  lzt::set_group_size(kernel_handle, 1, 1, 1);
  ze_group_count_t group_count = {1, 1, 1};

  uint8_t zero = 0;
  void *verify_value =
      lzt::allocate_device_memory(sizeof(uint64_t), 8, 0, 0, device, context);
  lzt::append_memory_set(cmd_list, verify_value, &zero, sizeof(uint64_t));
  lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);

  lzt::set_argument_value(kernel_handle, 0, sizeof(verify_value),
                          &verify_value);

  // Submit workload
  LOG_INFO << "Submitting " << submission_count << " kernel launches";
  for (uint64_t i = 0; i < submission_count; i++) {
    lzt::append_launch_function(cmd_list, kernel_handle, &group_count, nullptr,
                                0, nullptr);
  }

  // Try overflowing submission count
  for (uint64_t i = 0; i < std::numeric_limits<uint8_t>::max(); i++) {
    lzt::append_launch_function(cmd_list, kernel_handle, &group_count, nullptr,
                                0, nullptr);
  }
  LOG_INFO << "Submissions done";

  lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);

  // Submit workload to see whether there are any side effects
  size_t test_buffer_size = 1 << 27;
  void *verify_dev_buffer_1 =
      lzt::allocate_device_memory(test_buffer_size, 1, 0, 0, device, context);
  void *verify_dev_buffer_2 =
      lzt::allocate_device_memory(test_buffer_size, 1, 0, 0, device, context);
  void *verify_host_buffer =
      lzt::allocate_host_memory(test_buffer_size, 1, 0, nullptr, context);
  std::memset(verify_host_buffer, 0, test_buffer_size);

  uint8_t pattern_1 = 0xAB;
  uint8_t pattern_2 = 0xCD;
  lzt::append_memory_fill(cmd_list, verify_dev_buffer_1, &zero, sizeof(zero),
                          test_buffer_size, nullptr);
  lzt::append_memory_fill(cmd_list, verify_dev_buffer_2, &zero, sizeof(zero),
                          test_buffer_size, nullptr);
  lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);

  for (int i = 0; i < 10; ++i) {
    lzt::append_memory_fill(cmd_list, verify_dev_buffer_1, &pattern_1,
                            sizeof(pattern_1), test_buffer_size, nullptr);
    lzt::append_barrier(cmd_list);
    lzt::append_memory_copy(cmd_list, verify_dev_buffer_2, verify_dev_buffer_1,
                            test_buffer_size, nullptr);
    lzt::append_barrier(cmd_list);
    lzt::append_memory_copy(cmd_list, verify_host_buffer, verify_dev_buffer_2,
                            test_buffer_size, nullptr);
    lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);

    for (size_t j = 0; j < test_buffer_size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(verify_host_buffer)[j], pattern_1);
    }
    std::memset(verify_host_buffer, 0, test_buffer_size);

    lzt::append_memory_fill(cmd_list, verify_dev_buffer_2, &pattern_2,
                            sizeof(pattern_2), test_buffer_size, nullptr);
    lzt::append_barrier(cmd_list);
    lzt::append_memory_copy(cmd_list, verify_dev_buffer_1, verify_dev_buffer_2,
                            test_buffer_size, nullptr);
    lzt::append_barrier(cmd_list);
    lzt::append_memory_copy(cmd_list, verify_host_buffer, verify_dev_buffer_1,
                            test_buffer_size, nullptr);
    lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);

    for (size_t j = 0; j < test_buffer_size; j++) {
      EXPECT_EQ(static_cast<uint8_t *>(verify_host_buffer)[j], pattern_2);
    }
    std::memset(verify_host_buffer, 0, test_buffer_size);
  }

  // Verify
  uint64_t read_value = 0;
  lzt::append_memory_copy(cmd_list, &read_value, verify_value, sizeof(uint64_t),
                          nullptr);
  lzt::synchronize_command_list_host(cmd_list, UINT64_MAX);
  EXPECT_EQ(read_value, submission_count + UINT8_MAX);

  // Cleanup
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(kernel_handle);
  lzt::destroy_module(module_handle);
  lzt::free_memory(context, verify_value);
  lzt::destroy_context(context);
}

} // namespace