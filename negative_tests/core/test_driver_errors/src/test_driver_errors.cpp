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
TEST(InitNegativeTests,
     GivenInvalidFlagValueWhileCallingzeInitThenInvalidEnumerationIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ENUMERATION,
            zeInit(static_cast<ze_init_flag_t>(ZE_RESULT_ERROR_UNKNOWN)));
}
TEST(DriverGetNegativeTests,
     GivenzeDriverGetIsCalledBeforezeInitThenUninitialiZedIsReturned) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_UNINITIALIZED, zeDriverGet(&pCount, nullptr));
}

TEST(
    DriverGetNegativeTests,
    GivenInvalidCountPointerUsedWhileCallingzeDriverGetThenInvalidNullPointerIsReturned) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            zeDriverGet(nullptr, nullptr));
}
#ifdef ZE_API_VERSION_CURRENT_M
TEST(
  zeInitDriversNegativeTests,
  GivenCallToZeInitDriversWithNoFlagsThenExpectFailure) {

  uint32_t pCount = 0;
  ze_init_driver_type_desc_t desc = {ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC};
  desc.pNext = nullptr;

  // Test with no flags
  desc.flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ENUMERATION, zeInitDrivers(&pCount, nullptr, &desc));
  EXPECT_EQ(pCount, 0);
}

TEST(
  zeInitDriversNegativeTests,
  GivenCallToZeInitDriversWithNullPointerCountThenExpectFailure) {

  uint32_t pCount = 0;
  ze_init_driver_type_desc_t desc = {ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC};
  desc.pNext = nullptr;

  // Test with nullptr pCount
  desc.flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER, zeInitDrivers(nullptr, nullptr, &desc));
}

TEST(
  zeInitDriversNegativeTests,
  GivenCallToZeInitDriversWithNullPointerDescThenExpectFailure) {

  uint32_t pCount = 0;
  ze_init_driver_type_desc_t desc = {ZE_STRUCTURE_TYPE_INIT_DRIVER_TYPE_DESC};
  desc.pNext = nullptr;

  // Test with nullptr Desc
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER, zeInitDrivers(&pCount, nullptr, nullptr));
  EXPECT_EQ(pCount, 0);
}
#endif

} // namespace
