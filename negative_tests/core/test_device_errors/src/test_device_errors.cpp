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
} // namespace
