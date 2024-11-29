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

TEST(SysmanInitTests,
     GivenZeInitWithSysmanDisabledWhenZesInitIsCalledThenSuccessIsReturned) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char disable_sysman_env[] = "ZES_ENABLE_SYSMAN=0";
    putenv(disable_sysman_env);
  }
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t ze_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zes_driver_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&ze_driver_count, nullptr));
  EXPECT_GT(ze_driver_count, 0);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zes_driver_count, nullptr));
  EXPECT_GT(zes_driver_count, 0);
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char enable_sysman_env[] = "ZES_ENABLE_SYSMAN=1";
    putenv(enable_sysman_env);
  }
}

} // namespace
