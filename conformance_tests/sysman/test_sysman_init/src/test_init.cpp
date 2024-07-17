/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

TEST(
    SysmanInitTests,
    GivenSysmanEnableMacroIsDisabledAndCoreInitializedFirstWhenSysmanInitializedThenzesDriverGetWorks) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=0";
  putenv(sys_env);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t zeInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&zeInitCount, nullptr));
  ASSERT_GT(zeInitCount, 0);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zesInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zesInitCount, nullptr));
  ASSERT_GT(zesInitCount, 0);
}

TEST(SysmanInitTests, GivenSysmanInitializedThenCallingCoreInitSucceeds) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t zesInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&zesInitCount, nullptr));
  ASSERT_GT(zesInitCount, 0);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t zeInitCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&zeInitCount, nullptr));
  ASSERT_GT(zeInitCount, 0);
}

TEST(SysmanInitTests,
     GivenCoreNotInitializedWhenSysmanInitializedThenzesDriverGetWorks) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t pCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&pCount, nullptr));
  ASSERT_GT(pCount, 0);
}

TEST(SysmanInitTests,
     GivenEnableSysmanEnvSetWhenCoreInitializedThenzesDriverGetWorks) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zeInit(0));
  uint32_t pCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&pCount, nullptr));
  ASSERT_GT(pCount, 0);
}

} // namespace
