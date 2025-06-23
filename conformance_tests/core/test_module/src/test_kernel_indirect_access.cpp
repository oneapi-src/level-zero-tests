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

LZT_TEST(zeKernelIndirectAccessTests,
         GivenKernelGetIndirectAccessFlagsCorrectFlagsAreReturned) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  std::vector<ze_kernel_indirect_access_flags_t> indirectAccessFlags = {
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
          ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
          ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE |
          ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED,
      ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST |
          ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE |
          ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED};

  for (auto inputFlags : indirectAccessFlags) {

    auto module = lzt::create_module(context, device, "module_add.spv",
                                     ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
    auto kernel = lzt::create_function(module, "module_add_constant");

    lzt::kernel_set_indirect_access(kernel, inputFlags);

    ze_kernel_indirect_access_flags_t outputFlags =
        ZE_KERNEL_INDIRECT_ACCESS_FLAG_FORCE_UINT32;
    lzt::kernel_get_indirect_access(kernel, &outputFlags);

    EXPECT_EQ(inputFlags, outputFlags);

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
  }

  lzt::destroy_context(context);
}