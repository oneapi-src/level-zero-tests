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
    GivenZesInitAndZeInitWithSysmanEnabledWhenSysmanApiIsCalledWithZeDeviceHandleThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<ze_driver_handle_t> ze_drivers = lzt::get_all_driver_handles();
  EXPECT_FALSE(ze_drivers.empty());
  std::vector<ze_device_handle_t> ze_devices =
      lzt::get_ze_devices(ze_drivers[0]);
  EXPECT_FALSE(ze_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zesDeviceEnumFrequencyDomains(
          static_cast<zes_device_handle_t>(ze_devices[0]), &count, nullptr));
}

} // namespace
