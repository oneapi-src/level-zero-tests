/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {
TEST(
    ModuleCreateNegativeTests,
    GivenInvalidDeviceHandleWhileCallingzeModuleCreateThenInvalidNullHandleIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeModuleCreate(lzt::get_default_context(), nullptr, nullptr,
                           nullptr, nullptr));
}
TEST(
    ModuleCreateNegativeTests,
    GivenInvalidModuleDescriptionOrModulePointerwhileCallingzeModuleCreateThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeModuleCreate(lzt::get_default_context(), device, nullptr, nullptr,
                           nullptr)); // Invalid module description
  const std::string filename = "ze_matrix_multiplication_errors.spv";
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file(filename);

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeModuleCreate(lzt::get_default_context(), device,
                           &module_description, nullptr,
                           nullptr)); // Invalid module pointer
}
TEST(
    ModuleCreateNegativeTests,
    GivenInvalidFileFormatwhileCallingzeModuleCreateThenInvalidEnumerationIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_module_handle_t module;
  const std::string filename = "ze_matrix_multiplication_errors.spv";
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file(filename);
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;

  module_description.pNext = nullptr;
  module_description.format =
      static_cast<ze_module_format_t>(ZE_MODULE_FORMAT_FORCE_UINT32);
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ENUMERATION,
            zeModuleCreate(lzt::get_default_context(), device,
                           &module_description, &module, nullptr));
}
TEST(
    ModuleDestroyNegativeTests,
    GivenInvalidModuleHandleWhileCallingzeModuleDestroyThenInvalidNullHandleIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE, zeModuleDestroy(nullptr));
}

class ModuleNegativeLocalMemoryTests : public ::testing::Test {

public:
  ModuleNegativeLocalMemoryTests() {
    device_ = lzt::zeDevice::get_instance()->get_device();
    module_ = lzt::create_module(device_, "local_memory_argument_kernel.spv");
    cmd_list_ = lzt::create_command_list(device_);
    cmd_queue_ = lzt::create_command_queue();
  }

  ~ModuleNegativeLocalMemoryTests() {
    lzt::destroy_module(module_);
    lzt::destroy_command_list(cmd_list_);
    lzt::destroy_command_queue(cmd_queue_);
  }

protected:
  ze_device_handle_t device_;
  ze_module_handle_t module_;
  ze_command_list_handle_t cmd_list_;
  ze_command_queue_handle_t cmd_queue_;
};

TEST_F(
    ModuleNegativeLocalMemoryTests,
    GivenKernelWithSharedLocalMemoryLargerThanMaxSharedLocalMemorySizeThenOutOfMemoryErrorIsReturned) {
  std::string kernel_name = "single_local";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  auto device_compute_properties = lzt::get_compute_properties(device_);

  const int local_size =
      (device_compute_properties.maxSharedLocalMemory + 1) * sizeof(uint8_t);
  auto buff = lzt::allocate_host_memory(local_size);

  arg.arg_size = local_size;
  arg.arg_value = nullptr;
  args.push_back(arg);

  arg.arg_size = sizeof(buff);
  arg.arg_value = &buff;
  args.push_back(arg);

  ze_kernel_handle_t function = lzt::create_function(module_, kernel_name);
  ze_command_list_handle_t cmdlist = lzt::create_command_list(device_);
  ze_command_queue_handle_t cmdq = lzt::create_command_queue(device_);
  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSuggestGroupSize(function, 1, 1, 1, &group_size_x,
                                     &group_size_y, &group_size_z));

  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));

  int i = 0;
  for (auto arg : args) {
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeKernelSetArgumentValue(function, i++, arg.arg_size, arg.arg_value));
  }

  ze_group_count_t thread_group_dimensions;
  thread_group_dimensions.groupCountX = 1;
  thread_group_dimensions.groupCountY = 1;
  thread_group_dimensions.groupCountZ = 1;

  EXPECT_EQ(ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY,
            zeCommandListAppendLaunchKernel(cmdlist, function,
                                            &thread_group_dimensions, nullptr,
                                            0, nullptr));

  lzt::free_memory(buff);
}

} // namespace
