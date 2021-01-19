/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

TEST(
    PINTests,
    GivenModuleCreatedWithProfileFlagsWhenGettingKernelProfileInfoThenValidPropertiesAreReturned) {

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  std::stringstream build_string;
  build_string << "-zet-profile-flags " << std::hex
               << ZET_PROFILE_FLAG_FREE_REGISTER_INFO;

  auto module = lzt::create_module(context, device, "profile_module.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV,
                                   build_string.str().c_str(), nullptr);

  auto kernel = lzt::create_function(module, "blackscholes");

  zet_profile_properties_t kernel_profile_properties = {};
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zetKernelGetProfileInfo(kernel, &kernel_profile_properties));

  EXPECT_EQ(kernel_profile_properties.flags,
            ZET_PROFILE_FLAG_FREE_REGISTER_INFO);

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}

} // namespace
