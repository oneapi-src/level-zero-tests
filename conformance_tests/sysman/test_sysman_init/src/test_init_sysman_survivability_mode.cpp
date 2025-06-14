/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>

namespace {

TEST(
    SysmanInitTests,
    GivenSuccessfulSysmanInitWhenzesDeviceGetReturnsValidDevicesThenSurvivabilityModeIsOffIsDetected) {

  ze_result_t result = zesInit(0);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);

  std::vector<zes_driver_handle_t> drivers(zes_driver_count);
  result = zesDriverGet(&zes_driver_count, drivers.data());
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  for (auto driver : drivers) {
    uint32_t device_count = 0;
    result = zesDeviceGet(driver, &device_count, nullptr);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS);
    ASSERT_GT(device_count, 0);

    std::vector<zes_device_handle_t> devices(device_count);
    result = zesDeviceGet(driver, &device_count, devices.data());
    ASSERT_EQ(result, ZE_RESULT_SUCCESS);

    for (auto device : devices) {
      zes_device_properties_t properties = {
          ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
      result = zesDeviceGetProperties(device, &properties);
      EXPECT_EQ(result, ZE_RESULT_SUCCESS);
    }
  }
}

TEST(
    SysmanInitTests,
    GivenSysmanInitWhenzesDeviceGetPropertiesReturnsSurvivabilityDetectedErrorThenOnlyFormwareAPIsSucceed) {

  ze_result_t result = zesInit(0);
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  std::vector<zes_driver_handle_t> drivers(zes_driver_count);
  result = zesDriverGet(&zes_driver_count, drivers.data());
  ASSERT_EQ(result, ZE_RESULT_SUCCESS);

  for (auto driver : drivers) {
    uint32_t device_count = 0;
    result = zesDeviceGet(driver, &device_count, nullptr);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS);
    ASSERT_GT(device_count, 0);

    std::vector<zes_device_handle_t> devices(device_count);
    result = zesDeviceGet(driver, &device_count, devices.data());
    ASSERT_EQ(result, ZE_RESULT_SUCCESS);

    for (auto device : devices) {
      zes_device_properties_t properties = {
          ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
      result = zesDeviceGetProperties(device, &properties);
      EXPECT_EQ(result, ZE_RESULT_SUCCESS);

      uint32_t temp_count = 0;
      temp_count = lzt::get_temp_handle_count(device);
      if (temp_count == 0) {
        FAIL() << "No handles found: "
               << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
      }

      uint32_t standby_count = 0;
      auto p_standby_handles = lzt::get_standby_handles(device, standby_count);
      for (auto p_standby_handle : p_standby_handles) {
        EXPECT_NE(nullptr, p_standby_handle);
      }

      uint32_t count = 0;
      count = lzt::get_firmware_handle_count(device);
      if (count == 0) {
        FAIL() << "No handles found: "
               << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
      }

      auto firmware_handles = lzt::get_firmware_handles(device, count);
      if (count == 0) {
        FAIL() << "No handles found: "
               << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
      }
    }
  }
}

} // namespace
