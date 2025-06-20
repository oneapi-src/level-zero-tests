/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include <level_zero/ze_api.h>
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

namespace {

LZT_TEST(zeInitTests,
         GivenVpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturned) {
  ze_result_t result = zeInit(ZE_INIT_FLAG_VPU_ONLY);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNINITIALIZED);
}

} // namespace
