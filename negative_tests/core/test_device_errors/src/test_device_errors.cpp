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
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_HANDLE),
            uint64_t(zeDeviceGet(nullptr, &pCount, nullptr)));
}

TEST(
    DeviceGetNegativeTests,
    GivenInvalidOutputCountPointerWhileCallingzeDeviceGetThenInvalidNullPointerIsReturned) {
  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
              uint64_t(zeDeviceGet(driver, nullptr, nullptr)));
  }
}
} // namespace
