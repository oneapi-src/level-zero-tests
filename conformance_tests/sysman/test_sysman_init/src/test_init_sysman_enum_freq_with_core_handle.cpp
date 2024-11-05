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
    GivenSysmanInitializationDoneUsingZesInitFollowedByZeInitAlongWithSysmanFlagEnabledThenWhenSysmanApiZesDeviceEnumFrequencyDomainsIsCalledWithCoreHandleThenUninitializedErrorIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  uint32_t driver_count = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, nullptr));
  ASSERT_GT(driver_count, 0);
  std::vector<zes_driver_handle_t> drivers(driver_count);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, drivers.data()));

  uint32_t device_count = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDeviceGet(drivers[0], &device_count, nullptr));
  std::vector<zes_device_handle_t> devices(device_count);
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGet(drivers[0], &device_count, devices.data()));

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED,
            zesDeviceEnumFrequencyDomains(devices[0], &count, nullptr));
}

} // namespace
