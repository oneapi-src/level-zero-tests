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