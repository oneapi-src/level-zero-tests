/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

TEST(
    DeviceGetNegativeTests,
    GivenInvalidDriverHandleWhileCallingzeDeviceGetThenInvalidHandleIsReturned) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeDeviceGet(nullptr, &pCount, nullptr));
}

TEST(
    DeviceGetNegativeTests,
    GivenInvalidOutputCountPointerWhileCallingzeDeviceGetThenInvalidNullPointerIsReturned) {
  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
              zeDeviceGet(driver, nullptr, nullptr));
  }
}

TEST(DeviceNegativeTests, GivenInvalidStypeWhen) {
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  ze_device_properties_t props;
  props.pNext = nullptr;
  props.stype = (ze_structure_type_t)0xaaaa;
  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            zeDeviceGetProperties(device, &props));

  props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &props));

  ze_device_properties_t props2;
  props.pNext = &props2;
  props2.pNext = nullptr;
  props2.stype = (ze_structure_type_t)0xaaaa;

  ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
            zeDeviceGetProperties(device, &props));
}

} // namespace
