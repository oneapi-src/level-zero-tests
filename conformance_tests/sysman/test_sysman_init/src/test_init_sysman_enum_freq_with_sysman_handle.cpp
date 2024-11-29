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
    GivenZesInitAndZeInitWithSysmanEnabledWhenSysmanApiIsCalledWithZesDeviceThenSuccessIsReturned) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));

  std::vector<zes_driver_handle_t> zes_drivers =
      lzt::get_all_zes_driver_handles();
  EXPECT_FALSE(zes_drivers.empty());
  std::vector<zes_device_handle_t> zes_devices =
      lzt::get_zes_devices(zes_drivers[0]);
  EXPECT_FALSE(zes_devices.empty());

  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(zes_devices[0], &count, nullptr));
}

} // namespace
