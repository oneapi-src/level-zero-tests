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

TEST(SysmanInitTests, GivenZesInitWhenZeInitIsCalledThenSuccessIsReturned) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t ze_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&ze_driver_count, nullptr));
  EXPECT_GT(ze_driver_count, 0);
}

} // namespace
