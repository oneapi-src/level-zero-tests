/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
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
using lzt::to_u32;

class CooperativeKernelTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<uint32_t, bool>> {
protected:
  void RunGivenCooperativeKernelWhenAppendingLaunchCooperativeKernelTest(
      bool is_shared_system);
};

void CooperativeKernelTests::
    RunGivenCooperativeKernelWhenAppendingLaunchCooperativeKernelTest(
        bool is_shared_system) {
  uint32_t max_group_count = 0;
  ze_module_handle_t module = nullptr;
  ze_kernel_handle_t kernel = nullptr;
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto context = lzt::create_context(driver);
  ze_command_queue_flags_t flags = 0;
  auto mode = ZE_COMMAND_QUEUE_MODE_DEFAULT;
  auto priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL;
  int ordinal = -1;

  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device);
  for (size_t i = 0; i < command_queue_group_properties.size(); i++) {
    if (command_queue_group_properties[i].flags &
        ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS) {
      ordinal = to_int(i);
      break;
    }
  }

  if (ordinal < 0) {
    LOG_WARNING << "No command queues that support cooperative kernels";
    GTEST_SKIP();
  }
  auto is_immediate = std::get<1>(GetParam());
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, flags, mode, priority, 0, to_u32(ordinal), 0, is_immediate);

  // Set up input vector data
  const size_t data_size = 4096;
  uint64_t kernel_data[data_size] = {0};
  void *input_data = lzt::allocate_shared_memory_with_allocator_selector(
      sizeof(uint64_t) * data_size, 1, 0, 0, device, context, is_shared_system);

  memcpy(input_data, kernel_data, data_size * sizeof(uint64_t));

  uint32_t row_num = std::get<0>(GetParam());
  uint32_t groups_x = 1;

  module = lzt::create_module(context, device, "cooperative_kernel.spv",
                              ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  kernel = lzt::create_function(module, "cooperative_kernel");

  // Use a small group size in order to use more groups
  // in order to stress cooperation
  zeKernelSetGroupSize(kernel, 1, 1, 1);
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSuggestMaxCooperativeGroupCount(kernel, &groups_x));
  ASSERT_GT(groups_x, 0);
  // We've set the number of workgroups to the max
  // so check that it is sufficient for the input,
  // otherwise just clamp the test
  if (groups_x < row_num) {
    row_num = groups_x;
  }
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSetArgumentValue(kernel, 0, sizeof(input_data), &input_data));
  ASSERT_ZE_RESULT_SUCCESS(
      zeKernelSetArgumentValue(kernel, 1, sizeof(row_num), &row_num));

  ze_group_count_t args = {groups_x, 1, 1};
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchCooperativeKernel(
      cmd_bundle.list, kernel, &args, nullptr, 0, nullptr));

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  // Validate the kernel completed successfully and correctly
  uint64_t val = 0;
  for (uint32_t i = 0U; i <= row_num; i++) {
    val = i + row_num;
    ASSERT_EQ(static_cast<uint64_t *>(input_data)[i], val);
  }

  lzt::free_memory_with_allocator_selector(context, input_data,
                                           is_shared_system);

  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

LZT_TEST_P(
    CooperativeKernelTests,
    GivenCooperativeKernelWhenAppendingLaunchCooperativeKernelThenSuccessIsReturnedAndOutputIsCorrect) {
  RunGivenCooperativeKernelWhenAppendingLaunchCooperativeKernelTest(false);
}

LZT_TEST_P(
    CooperativeKernelTests,
    GivenCooperativeKernelWhenAppendingLaunchCooperativeKernelThenSuccessIsReturnedAndOutputIsCorrectWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  RunGivenCooperativeKernelWhenAppendingLaunchCooperativeKernelTest(true);
}

INSTANTIATE_TEST_SUITE_P(
    // 62 is the max row such that no calculation will overflow max uint64 value
    GroupNumbers, CooperativeKernelTests,
    ::testing::Combine(::testing::Values(0, 1, 5, 10, 50, 62),
                       ::testing::Bool()));

} // namespace
