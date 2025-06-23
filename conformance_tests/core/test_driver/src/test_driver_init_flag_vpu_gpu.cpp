/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include <level_zero/ze_api.h>
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

namespace {

LZT_TEST(
    zeInitTests,
    GivenVpuOnlyFlagThenGpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturnedAndAtLeastOneDriverHandleIsReturned) {
  ze_result_t result = zeInit(ZE_INIT_FLAG_VPU_ONLY);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  if (result == ZE_RESULT_SUCCESS) {
    auto drivers = lzt::get_all_driver_handles();
    EXPECT_GT(drivers.size(), 0);
    for (auto driver : drivers) {
      uint32_t version = lzt::get_driver_version(driver);
      LOG_INFO << "VPU Driver version: " << version;
    }
  }
  result = zeInit(ZE_INIT_FLAG_GPU_ONLY);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  if (result == ZE_RESULT_SUCCESS) {
    auto drivers = lzt::get_all_driver_handles();
    EXPECT_GT(drivers.size(), 0);
    for (auto driver : drivers) {
      uint32_t version = lzt::get_driver_version(driver);
      LOG_INFO << "GPU Driver version: " << version;
    }
  }
}

LZT_TEST(
    zeInitTests,
    GivenGpuOnlyFlagThenVpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturnedAndAtLeastOneDriverHandleIsReturned) {
  ze_result_t result = zeInit(ZE_INIT_FLAG_GPU_ONLY);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  if (result == ZE_RESULT_SUCCESS) {
    auto drivers = lzt::get_all_driver_handles();
    EXPECT_GT(drivers.size(), 0);
    for (auto driver : drivers) {
      uint32_t version = lzt::get_driver_version(driver);
      LOG_INFO << "VPU Driver version: " << version;
    }
  }
  result = zeInit(ZE_INIT_FLAG_VPU_ONLY);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  if (result == ZE_RESULT_SUCCESS) {
    auto drivers = lzt::get_all_driver_handles();
    EXPECT_GT(drivers.size(), 0);
    for (auto driver : drivers) {
      uint32_t version = lzt::get_driver_version(driver);
      LOG_INFO << "GPU Driver version: " << version;
    }
  }
}

} // namespace
