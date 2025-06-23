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
#include <chrono>
#include <fstream>
namespace lzt = level_zero_tests;
#include <level_zero/ze_api.h>

namespace {
constexpr uint32_t work_items_count = 64;
constexpr uint32_t thread_groups_count = 8;

enum TestTypes { test_printf, test_get_global_id, test_get_group_id };
std::string test_type_to_str(TestTypes test) {
  switch (test) {
  case test_printf:
    return "test_printf";
  case test_get_global_id:
    return "test_get_global_id";
  case test_get_group_id:
    return "test_get_group_id";
  default:
    return "";
  }
}
std::string cmdlist_type_to_str(bool is_immediate) {
  return is_immediate ? "_true_immediate" : "_false_immediate";
}

class CaptureOutput {
private:
  int orig_file_descriptor;
  int file_descriptor;
  int stream;
  std::string file_name;

public:
  enum { Stdout = 1, Stderr = 2 };

  CaptureOutput(int stream_) : stream(stream_) {
    orig_file_descriptor = dup(stream);
#if defined(__linux__)
    file_name = "/tmp/fileTempOXXXXXX";
    file_descriptor = mkstemp((char *)file_name.c_str());
#elif defined(_WIN32)
    file_name = std::tmpnam(nullptr);
    file_descriptor = creat(file_name.c_str(), _S_IREAD | _S_IWRITE);
#endif
    if (file_descriptor < 0) {
      LOG_ERROR << "Error creating file" << std::endl;
      return;
    }
    fflush(nullptr);
    dup2(file_descriptor, stream);
    close(file_descriptor);
  }

  ~CaptureOutput() {
    if (orig_file_descriptor != -1) {
      fflush(nullptr);
      dup2(orig_file_descriptor, stream);
      close(orig_file_descriptor);
      orig_file_descriptor = -1;
    }
    if (remove(file_name.c_str())) {
      LOG_WARNING << "Error removing file: " << file_name << std::endl;
    }
  }

  std::stringstream GetOutput() {
    if (orig_file_descriptor != -1) {
      fflush(nullptr);
      dup2(orig_file_descriptor, stream);
      close(orig_file_descriptor);
      orig_file_descriptor = -1;
    }
    std::ifstream input_file(file_name);
    std::stringstream buffer;
    buffer << input_file.rdbuf();
    return buffer;
  }
};

class zeCheckFunctions : public ::testing::Test {
protected:
  static ze_context_handle_t context;
  static ze_device_handle_t device;
  static ze_module_handle_t module;
  static ze_event_pool_handle_t event_pool;
  static ze_event_handle_t event0;
  static uint32_t *out_allocation;

  void SetUp() {
    context = lzt::get_default_context();
    device = lzt::get_default_device(lzt::get_default_driver());
    module = lzt::create_module(device, "cmdlist_and_functions.spv");
    out_allocation = (uint32_t *)lzt::allocate_host_memory(work_items_count *
                                                           sizeof(uint32_t));
    ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
                                            nullptr,
                                            ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
    event_pool = lzt::create_event_pool(context, event_pool_desc);
    ze_event_desc_t event_desc = {};
    event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
    event_desc.pNext = nullptr;
    event_desc.index = 0;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    event_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;

    event0 = lzt::create_event(event_pool, event_desc);
  }

  void TearDown() {
    lzt::destroy_event_pool(event_pool);
    lzt::destroy_event(event0);
    lzt::destroy_module(module);
    lzt::free_memory(out_allocation);
  }
};

bool validate_output(std::stringstream printf_result, uint32_t *gpu_result,
                     TestTypes test_type) {

  if (test_type == test_printf) {
    std::vector<std::string> templates;
    std::vector<std::string> outputs;
    for (uint32_t i = 0; i < work_items_count; i++) {
      templates.emplace_back("printf test_printf message only 1234");
    }
    std::string line;
    while (std::getline(printf_result, line))
      outputs.emplace_back(line);

    std::sort(templates.begin(), templates.end());
    std::sort(outputs.begin(), outputs.end());
    LOG_DEBUG << "templates:" << std::endl;
    for (auto elem : templates) {
      LOG_DEBUG << elem << std::endl;
    }
    LOG_DEBUG << "outputs:" << std::endl;
    for (auto elem : outputs) {
      LOG_DEBUG << elem << std::endl;
    }
    return templates == outputs;
  }

  if (test_type == test_get_global_id) {
    for (uint32_t i = 0; i < work_items_count; i++) {
      LOG_DEBUG << "GPU result " << gpu_result[i] << std::endl;
      if (gpu_result[i] != i) {
        return false;
      }
    }
    return true;
  }

  if (test_type == test_get_group_id) {
    constexpr uint32_t number_of_groups =
        work_items_count / thread_groups_count;
    for (uint32_t i = 0; i < number_of_groups; i++) {
      LOG_DEBUG << "GPU result " << gpu_result[i] << std::endl;
      if (gpu_result[i] >= number_of_groups) {
        return false;
      }
    }
    return true;
  }
  return false;
}

ze_context_handle_t zeCheckFunctions::context = nullptr;
ze_device_handle_t zeCheckFunctions::device = nullptr;
ze_module_handle_t zeCheckFunctions::module = nullptr;
ze_event_pool_handle_t zeCheckFunctions::event_pool = nullptr;
ze_event_handle_t zeCheckFunctions::event0 = nullptr;
uint32_t *zeCheckFunctions::out_allocation = nullptr;

class zeCheckMiscFunctionsTests
    : public zeCheckFunctions,
      public ::testing::WithParamInterface<std::tuple<TestTypes, bool>> {};

struct CombinationsTestNameSuffix {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
    return test_type_to_str(std::get<0>(info.param)) +
           cmdlist_type_to_str(std::get<1>(info.param));
  }
};

auto kernel_function_types =
    ::testing::Values(TestTypes::test_printf, TestTypes::test_get_global_id,
                      TestTypes::test_get_group_id);
auto immediate_types = ::testing::Values(true, false);
INSTANTIATE_TEST_CASE_P(zeCheckMiscFunctionsTestsInstantiate,
                        zeCheckMiscFunctionsTests,
                        ::testing::Combine(kernel_function_types,
                                           immediate_types),
                        CombinationsTestNameSuffix());

LZT_TEST_P(
    zeCheckMiscFunctionsTests,
    GivenFunctionInKernelWhenLaunchingKernelsThenFunctionWorksCorrectly) {

  const TestTypes test_type = std::get<0>(GetParam());
  bool is_immediate = std::get<1>(GetParam());
  std::string test_type_s = test_type_to_str(test_type);

  auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);
  lzt::reset_command_list(cmd_bundle.list);
  LOG_INFO << "Test type " << test_type_s << std::endl;

  uint32_t init_value = 4321;
  ze_kernel_handle_t function_handle =
      lzt::create_function(module, test_type_s);
  CaptureOutput capture_stdout(1);

  if (is_immediate) {
    lzt::append_memory_fill(cmd_bundle.list, out_allocation, &init_value,
                            sizeof(init_value),
                            work_items_count * sizeof(uint32_t), event0);
    LOG_INFO << "Synchronizing... memory fill" << std::endl;
    lzt::event_host_synchronize(event0, UINT64_MAX);
    lzt::append_reset_event(cmd_bundle.list, event0);
    ze_group_count_t thread_group_dimensions = {thread_groups_count, 1, 1};

    ze_kernel_handle_t function_handle =
        lzt::create_function(module, test_type_s);
    if (test_type != TestTypes::test_printf) {
      lzt::set_argument_value(function_handle, 0, sizeof(out_allocation),
                              &out_allocation);
    }
    lzt::set_group_size(function_handle, work_items_count / thread_groups_count,
                        1, 1);
    lzt::append_barrier(cmd_bundle.list, event0);
    lzt::event_host_synchronize(event0, UINT64_MAX);
    lzt::append_reset_event(cmd_bundle.list, event0);
    LOG_INFO << "Executing command list..." << std::endl;
    lzt::append_launch_function(cmd_bundle.list, function_handle,
                                &thread_group_dimensions, nullptr, 0, nullptr);
    LOG_INFO << "Synchronizing..." << std::endl;
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::append_memory_fill(cmd_bundle.list, out_allocation, &init_value,
                            sizeof(init_value),
                            work_items_count * sizeof(uint32_t), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr);
    if (test_type != TestTypes::test_printf) {
      lzt::set_argument_value(function_handle, 0, sizeof(out_allocation),
                              &out_allocation);
    }
    lzt::set_group_size(function_handle, work_items_count / thread_groups_count,
                        1, 1);
    ze_group_count_t thread_group_dimensions = {thread_groups_count, 1, 1};
    lzt::append_barrier(cmd_bundle.list, nullptr);
    lzt::append_launch_function(cmd_bundle.list, function_handle,
                                &thread_group_dimensions, nullptr, 0, nullptr);

    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    LOG_INFO << "Executing command list..." << std::endl;
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
    LOG_INFO << "Synchronizing..." << std::endl;
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }
  EXPECT_TRUE(
      validate_output(capture_stdout.GetOutput(), out_allocation, test_type));

  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(function_handle);
}

} // namespace
