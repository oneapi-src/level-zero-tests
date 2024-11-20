/*
 *
 * Copyright (C) 2019 Intel Corporation
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

TEST(
    zeInitTests,
    GivenDriverWasAlreadyInitializedWhenInitializingDriverThenSuccessIsReturned) {
  for (int i = 0; i < 5; ++i) {
    lzt::ze_init();
  }
}

TEST(zeDriverGetDriverVersionTests,
     GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned) {

  lzt::ze_init();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    uint32_t version = lzt::get_driver_version(driver);
    LOG_INFO << "Driver version: " << version;
  }
}

TEST(zeDriverGetApiVersionTests,
     GivenValidDriverWhenRetrievingApiVersionThenValidApiVersionIsReturned) {
  lzt::ze_init();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    ze_api_version_t api_version = lzt::get_api_version(driver);
    LOG_INFO << "API version: " << api_version;
  }
}

TEST(zeDriverGetIPCPropertiesTests,
     GivenValidDriverWhenRetrievingIPCPropertiesThenValidPropertiesAreRetured) {
  lzt::ze_init();
  auto drivers = lzt::get_all_driver_handles();
  ASSERT_GT(drivers.size(), 0);
  for (auto driver : drivers) {
    lzt::get_ipc_properties(driver);
  }
}

TEST(zeDriverGetLastErrorDescription,
     GivenValidDriverWhenRetreivingErrorDescriptionThenValidStringIsReturned) {
  lzt::ze_init();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    lzt::get_last_error_description(driver);
  }
}

#ifdef ZE_API_VERSION_CURRENT_M
TEST(
  zeInitDrivers,
  GivenCallToZeInitDriversWithAllCombinationsOfFlagsThenExpectAtLeastOneValidDriverHandle) {

  uint32_t pCount = 0;
  ze_init_driver_type_desc_t desc = {ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC};
  desc.pNext = nullptr;

  // Test with GPU only
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_GPU;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GE(pCount, 0);

  // Test with NPU only
  pCount = 0;
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_NPU;
  auto result = zeInitDrivers(&pCount, nullptr, &desc);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  EXPECT_GE(pCount, 0);

  // Test with GPU and NPU
  pCount = 0;
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_GPU | ZE_INIT_DRIVER_TYPE_FLAG_NPU;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GT(pCount, 0);

  // Test with all flags
  pCount = 0;
  desc.flags = UINT32_MAX;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GT(pCount, 0);
}

TEST(
  zeInitDrivers,
  GivenCallToZeInitDriversAndZeInitWithAllCombinationsOfFlagsThenExpectAtLeastOneValidDriverHandle) {

  uint32_t pCount = 0;
  ze_init_driver_type_desc_t desc = {ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC};
  desc.pNext = nullptr;

  // Test with GPU only
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_GPU;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GE(pCount, 0);
  lzt::ze_init(ZE_INIT_FLAG_GPU_ONLY);
  auto drivers = lzt::get_all_driver_handles();
  EXPECT_GE(drivers.size(), 0);

  // Test with NPU only
  pCount = 0;
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_NPU;
  auto result = zeInitDrivers(&pCount, nullptr, &desc);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
  EXPECT_GE(pCount, 0);
  lzt::ze_init(ZE_INIT_FLAG_VPU_ONLY);
  drivers = lzt::get_all_driver_handles();
  EXPECT_GE(drivers.size(), 0);

  // Test with GPU and NPU
  pCount = 0;
  desc.flags = ZE_INIT_DRIVER_TYPE_FLAG_GPU | ZE_INIT_DRIVER_TYPE_FLAG_NPU;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GT(pCount, 0);
  lzt::ze_init(ZE_INIT_FLAG_GPU_ONLY | ZE_INIT_FLAG_VPU_ONLY);
  drivers = lzt::get_all_driver_handles();
  EXPECT_GT(drivers.size(), 0);

  // Test with all flags
  pCount = 0;
  desc.flags = UINT32_MAX;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_GT(pCount, 0);
  lzt::ze_init(0);
  drivers = lzt::get_all_driver_handles();
  EXPECT_GT(drivers.size(), 0);
}
#endif

} // namespace
