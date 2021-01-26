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
  EXPECT_EQ(
      uint64_t(ZE_RESULT_ERROR_INVALID_ENUMERATION),
      uint64_t(zeInit(static_cast<ze_init_flag_t>(ZE_RESULT_ERROR_UNKNOWN))));
}
TEST(DriverGetNegativeTests,
     GivenzeDriverGetIsCalledBeforezeInitThenUninitialiZedIsReturned) {
  uint32_t pCount = 0;
  uint64_t errorExpected = uint64_t(ZE_RESULT_ERROR_UNINITIALIZED);
  uint64_t errorRecieved = uint64_t(zeDriverGet(&pCount, nullptr));
  EXPECT_EQ(errorExpected, errorRecieved);
}

TEST(
    DriverGetNegativeTests,
    GivenInvalidCountPointerUsedWhileCallingzeDriverGetThenInvalidNullPointerIsReturned) {
  lzt::ze_init();
  EXPECT_EQ(uint64_t(ZE_RESULT_ERROR_INVALID_NULL_POINTER),
            uint64_t(zeDriverGet(nullptr, nullptr)));
}
} // namespace
