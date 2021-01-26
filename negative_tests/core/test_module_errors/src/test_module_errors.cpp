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
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeModuleCreate(lzt::get_default_context(), nullptr,
                                    nullptr, nullptr, nullptr)));
}
TEST(
    ModuleCreateNegativeTests,
    GivenInvalidModuleDescriptionOrModulePointerwhileCallingzeModuleCreateThenInvalidNullPointerIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeModuleCreate(lzt::get_default_context(), device, nullptr,
                                    nullptr,
                                    nullptr))); // Invalid module description
  const std::string filename = "ze_matrix_multiplication.spv";
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file(filename);

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeModuleCreate(lzt::get_default_context(), device,
                                    &module_description, nullptr,
                                    nullptr))); // Invalid module pointer
}
TEST(
    ModuleCreateNegativeTests,
    GivenInvalidFileFormatwhileCallingzeModuleCreateThenInvalidEnumerationIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_module_handle_t module;
  const std::string filename = "ze_matrix_multiplication.spv";
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
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_ENUMERATION),
            uint64_t(zeModuleCreate(lzt::get_default_context(), device,
                                    &module_description, &module, nullptr)));
}
TEST(
    ModuleDestroyNegativeTests,
    GivenInvalidModuleHandleWhileCallingzeModuleDestroyThenInvalidNullHandleIsReturned) {
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeModuleDestroy(nullptr)));
}

} // namespace
