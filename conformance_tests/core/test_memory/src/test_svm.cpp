/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

class SharedSystemMemoryTests
    : public testing::TestWithParam<
          std::tuple<std::pair<bool, bool>, bool, bool, size_t>> {
protected:
  void SetUp() override {
    device = lzt::zeDevice::get_instance()->get_device();

    bool is_dst_shared_system = std::get<0>(GetParam()).first;
    bool is_src_shared_system = std::get<0>(GetParam()).second;
    if (is_dst_shared_system || is_src_shared_system) {
      SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
    }
  }

  ze_device_handle_t device;
};

LZT_TEST_P(
    SharedSystemMemoryTests,
    GivenSharedSystemMemoryAllocationsAsKernelArgumentsWhenKernelExecutesThenValuesAreCorrect) {
  bool is_dst_shared_system = std::get<0>(GetParam()).first;
  bool is_src_shared_system = std::get<0>(GetParam()).second;
  bool use_atomic_kernel = std::get<1>(GetParam());
  bool use_immediate_cmdlist = std::get<2>(GetParam());
  size_t buffer_size = std::get<3>(GetParam());

  constexpr size_t group_size = 32;
  ASSERT_EQ(buffer_size % (sizeof(int) * group_size), 0);

  constexpr int source_value = 1234;
  constexpr int add_value = 5678;
  const size_t num_elements = buffer_size / sizeof(int);

  void *result = lzt::allocate_shared_memory_with_allocator_selector(
      buffer_size, 1, 0, 0, device, is_dst_shared_system);
  void *source = lzt::allocate_shared_memory_with_allocator_selector(
      buffer_size, 1, 0, 0, device, is_src_shared_system);

  memset(result, 0, buffer_size);
  int *source_as_int = reinterpret_cast<int *>(source);
  for (size_t i = 0; i < num_elements; i++) {
    source_as_int[i] = source_value;
  }

  ze_module_handle_t module = lzt::create_module(device, "memory_add.spv");

  const char *funcion_name =
      use_atomic_kernel ? "memory_atomic_add" : "memory_add";
  ze_kernel_handle_t function = lzt::create_function(module, funcion_name);
  lzt::set_group_size(function, group_size, 1, 1);

  lzt::set_argument_value(function, 0, sizeof(result), &result);
  lzt::set_argument_value(function, 1, sizeof(source), &source);
  lzt::set_argument_value(function, 2, sizeof(add_value), &add_value);

  lzt::zeCommandBundle cmd_bundle =
      lzt::create_command_bundle(use_immediate_cmdlist);

  const uint32_t group_count_x = buffer_size / (sizeof(int) * group_size);
  ze_group_count_t thread_group_dimensions = {group_count_x, 1, 1};
  lzt::append_launch_function(cmd_bundle.list, function,
                              &thread_group_dimensions, nullptr, 0, nullptr);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  int *result_as_int = reinterpret_cast<int *>(result);
  for (size_t i = 0; i < num_elements; i++) {
    EXPECT_EQ(result_as_int[i], source_value + add_value) << "index = " << i;
  }

  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(function);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(source, is_src_shared_system);
  lzt::free_memory_with_allocator_selector(result, is_dst_shared_system);
}

LZT_TEST_P(
    SharedSystemMemoryTests,
    GivenSharedSystemMemoryAllocationsAsKernelArgumentsWhenCooperativeKernelExecutesThenValueIsCorrect) {
  int ordinal = -1;
  auto command_queue_group_properties =
      lzt::get_command_queue_group_properties(device);
  for (int i = 0; i < command_queue_group_properties.size(); i++) {
    if (command_queue_group_properties[i].flags &
        ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS) {
      ordinal = i;
      break;
    }
  }
  if (ordinal < 0) {
    LOG_WARNING << "No command queues that support cooperative kernels";
    GTEST_SKIP();
  }

  const bool is_dst_shared_system = std::get<0>(GetParam()).first;
  const bool is_src_shared_system = std::get<0>(GetParam()).second;
  const bool use_atomic_kernel = std::get<1>(GetParam());
  const bool use_immediate_cmdlist = std::get<2>(GetParam());
  const size_t buffer_size = std::get<3>(GetParam());
  const size_t num_elements = buffer_size / sizeof(int);

  void *input = lzt::allocate_shared_memory_with_allocator_selector(
      buffer_size, 1, 0, 0, device, is_src_shared_system);
  // void *output = lzt::allocate_shared_memory_with_allocator_selector(
  //     buffer_size, 1, 0, 0, device, is_dst_shared_system);

  int *input_as_int = reinterpret_cast<int *>(input);
  for (size_t i = 0; i < num_elements; i++) {
    input_as_int[i] = 1;
  }

  ze_module_handle_t module =
      lzt::create_module(device, "cooperative_reduction.spv");
  const char *function_name = use_atomic_kernel ? "cooperative_reduction_atomic"
                                                : "cooperative_reduction";
  ze_kernel_handle_t function = lzt::create_function(module, function_name);

  auto compute_properties = lzt::get_compute_properties(device);

  LOG_INFO << "Num elements: " << num_elements;
  uint32_t suggested_group_count = 1;
  ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestMaxCooperativeGroupCount(
      function, &suggested_group_count));
  ASSERT_GT(suggested_group_count, 0);
  uint32_t group_count = (num_elements < suggested_group_count)
                             ? num_elements
                             : suggested_group_count;
  LOG_INFO << "Group count: " << group_count;

  void *shared_buffer = lzt::allocate_shared_memory_with_allocator_selector(
      group_count * sizeof(int), 1, 0, 0, device, is_dst_shared_system);

  int result = 0;

  uint32_t group_size = num_elements / group_count;
  if (group_size > compute_properties.maxTotalGroupSize) {
    group_size = compute_properties.maxTotalGroupSize;
  }
  LOG_INFO << "Group size: " << group_size;
  // Clamp test to maximum supported work size
  uint32_t test_range = group_size * group_count;
  LOG_INFO << "Test range: " << test_range;

  lzt::set_group_size(function, group_size, 1, 1);
  lzt::set_argument_value(function, 0, sizeof(input), &input);
  lzt::set_argument_value(function, 1, sizeof(result), &result);
  lzt::set_argument_value(function, 2, sizeof(shared_buffer), &shared_buffer);
  lzt::set_argument_value(function, 3, sizeof(group_size), &group_size);

  lzt::zeCommandBundle cmd_bundle = lzt::create_command_bundle(
      lzt::get_default_context(), device, 0, ordinal, use_immediate_cmdlist);

  ze_group_count_t thread_group_dimensions = {group_count, 1, 1};
  lzt::append_launch_cooperative_function(
      cmd_bundle.list, function, &thread_group_dimensions, nullptr, 0, nullptr);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  // int *output_as_int = reinterpret_cast<int *>(output);
  // for (size_t i = 0; i < test_range; i++) {
  //   EXPECT_EQ(output_as_int[i], i + group_count) << "index = " << i;
  // }

  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(function);
  lzt::destroy_module(module);

  lzt::free_memory_with_allocator_selector(shared_buffer, is_dst_shared_system);
  lzt::free_memory_with_allocator_selector(input, is_src_shared_system);
  // lzt::free_memory_with_allocator_selector(output, is_dst_shared_system);
}

struct SharedSystemMemoryTestsNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    std::stringstream ss;
    bool is_dst_shared_system = std::get<0>(info.param).first;
    bool is_src_shared_system = std::get<0>(info.param).second;
    bool use_atomic_kernel = std::get<1>(info.param);
    bool use_immediate_cmdlist = std::get<2>(info.param);
    size_t buffer_size = std::get<3>(info.param);

    const char *buffer_size_str = [](size_t size) -> const char * {
      switch (size) {
      case 0x80u:
        return "_128B";
      case 0x1000u:
        return "_4KB";
      case 0x1800u:
        return "_6KB";
      case 0x100000u:
        return "_1MB";
      case 0x100800u:
        return "_1MB2KB";
      case 0x4000'0000u:
        return "_1GB";
      case 0x4000'0800u:
        return "_1GB2KB";
      }
      return "";
    }(buffer_size);

    ss << (is_src_shared_system ? "SVM" : "USM");
    ss << "to";
    ss << (is_dst_shared_system ? "SVM" : "USM");
    ss << (use_atomic_kernel ? "_Atomic" : "_NonAtomic");
    ss << (use_immediate_cmdlist ? "_Immediate" : "_Regular");
    ss << buffer_size_str;

    return ss.str();
  }
};

INSTANTIATE_TEST_SUITE_P(
    ParamSVMAllocationTests, SharedSystemMemoryTests,
    testing::Combine(testing::Values(std::make_pair(true, false),
                                     std::make_pair(false, true),
                                     std::make_pair(true, true)),
                     testing::Bool(), testing::Bool(),
                     testing::Values(0x80u, 0x1000u, 0x1800u, 0x100000u,
                                     0x100800u, 0x4000'0000u, 0x4000'0800u)),
    SharedSystemMemoryTestsNameSuffix());
