/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

TEST(zeKernelGetNameTests, GivenKernelGetNameCorrectNameIsReturned) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  auto module = lzt::create_module(context, device, "module_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  std::string kernelName = "module_add_constant";
  auto kernel = lzt::create_function(module, kernelName);

  std::string outputKernelName = lzt::kernel_get_name_string(kernel);
  EXPECT_EQ(kernelName, outputKernelName);

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}

TEST(zeKernelMaxGroupSize, GivenKernelGetMaxGroupSize) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  auto module = lzt::create_module(context, device, "module_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV,
                                   "-ze-opt-large-register-file", nullptr);
  std::string kernelName = "module_add_constant";
  auto kernel = lzt::create_function(module, kernelName);

  ze_kernel_max_group_size_properties_ext_t workGroupProperties = {};
  workGroupProperties.stype =
      ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_EXT_PROPERTIES;
  workGroupProperties.maxGroupSize = 0;

  ze_kernel_properties_t kernelProperties = {};
  kernelProperties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  kernelProperties.pNext = &workGroupProperties;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeKernelGetProperties(kernel, &kernelProperties));

  LOG_INFO << "workGroupProperties.maxGroupSize = "
           << workGroupProperties.maxGroupSize;

  ze_device_compute_properties_t computeProperties = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetComputeProperties(device, &computeProperties));

  LOG_INFO << "computeProperties.maxTotalGroupSize = "
           << computeProperties.maxTotalGroupSize;

  EXPECT_LE(workGroupProperties.maxGroupSize,
            computeProperties.maxTotalGroupSize);
  EXPECT_NE(0, workGroupProperties.maxGroupSize);

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}