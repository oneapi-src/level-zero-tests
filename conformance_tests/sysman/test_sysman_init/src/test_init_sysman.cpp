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
     GivenCoreNotInitializedWhenSysmanInitializedThenzesDriverGetWorks) {
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesInit(0));
  uint32_t pCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDriverGet(&pCount, nullptr));
  ASSERT_GT(pCount, 0);
}

} // namespace
