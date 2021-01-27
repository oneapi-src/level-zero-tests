/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness_driver.hpp"
#include <level_zero/ze_api.h>

#include "gtest/gtest.h"

namespace level_zero_tests {

void ze_init() { ze_init(0); }

void ze_init(ze_init_flags_t init_flag) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInit(init_flag));
}

ze_driver_properties_t get_driver_properties(ze_driver_handle_t driver) {
  ze_driver_properties_t properties = {};
  properties.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
  properties.pNext = nullptr;
  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetProperties(driver, &properties));
  EXPECT_EQ(driver, driver_initial);
  return properties;
}

uint32_t get_driver_version(ze_driver_handle_t driver) {

  uint32_t driverVersion = 0;
  ze_driver_properties_t properties;

  properties.pNext = nullptr;
  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetProperties(driver, &properties));
  EXPECT_EQ(driver, driver_initial);
  driverVersion = properties.driverVersion;
  EXPECT_NE(driverVersion, 0);

  return driverVersion;
}

ze_api_version_t get_api_version(ze_driver_handle_t driver) {
  ze_api_version_t api_version;

  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetApiVersion(driver, &api_version));
  EXPECT_EQ(driver, driver_initial);
  return api_version;
}

ze_driver_ipc_properties_t get_ipc_properties(ze_driver_handle_t driver) {
  ze_driver_ipc_properties_t properties = {
      ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES};

  auto driver_initial = driver;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetIpcProperties(driver, &properties));
  EXPECT_EQ(driver, driver_initial);
  return properties;
}

}; // namespace level_zero_tests
