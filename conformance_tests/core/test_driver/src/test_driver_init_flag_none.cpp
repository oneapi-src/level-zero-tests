/*
 *
 * Copyright (C) 2019 Intel Corporation
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

TEST(zeInitTests, GivenNoneFlagWhenInitializingDriverThenSuccessIsReturned) {
  lzt::ze_init(0);
}

} // namespace
