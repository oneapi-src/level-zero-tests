/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "stress_common_func.hpp"
namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeRunDriverIn57bAddressSpace : public ::testing::Test {};

LZT_TEST_F(zeRunDriverIn57bAddressSpace, CheckIfKernelCanBeExecuted) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);
  constexpr uint32_t data_count = 100;
  uint32_t init_value_1 = 0xAAAAAAAA;
  uint32_t init_value_2 = 0x55555555;

  uint32_t *input_allocation =
      allocate_memory<uint32_t>(context, device, ZE_MEMORY_TYPE_SHARED,
                                data_count * sizeof(uint32_t), false);

  uint32_t *output_allocation =
      allocate_memory<uint32_t>(context, device, ZE_MEMORY_TYPE_SHARED,
                                data_count * sizeof(uint32_t), false);

  std::string kernel_name = "simple_test";
  ze_module_handle_t module_handle =
      lzt::create_module(context, device, "test_memory_reservation_57b.spv");
  ze_command_list_handle_t command_list =
      lzt::create_command_list(context, device, 0);
  ze_kernel_handle_t kernel_handle =
      lzt::create_function(module_handle, kernel_name);
  lzt::set_group_size(kernel_handle, data_count, 1, 1);
  lzt::set_argument_value(kernel_handle, 0, sizeof(input_allocation),
                          &input_allocation);
  lzt::set_argument_value(kernel_handle, 1, sizeof(output_allocation),
                          &output_allocation);
  lzt::append_memory_fill(command_list, input_allocation, &init_value_1,
                          sizeof(init_value_1), data_count * sizeof(uint32_t),
                          nullptr);
  lzt::append_memory_fill(command_list, output_allocation, &init_value_2,
                          sizeof(init_value_2), data_count * sizeof(uint32_t),
                          nullptr);

  lzt::append_barrier(command_list, nullptr);
  ze_group_count_t thread_group_dimensions = {data_count, 1, 1};
  lzt::append_launch_function(command_list, kernel_handle,
                              &thread_group_dimensions, nullptr, 0, nullptr);

  lzt::append_barrier(command_list, nullptr);
  ze_command_queue_handle_t command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  lzt::destroy_command_queue(command_queue);
  lzt::destroy_command_list(command_list);
  lzt::destroy_module(module_handle);
  for (uint32_t i = 0; i < data_count; i++) {
    EXPECT_EQ(init_value_1, output_allocation[i]);
  }
}
} // namespace
