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

LZT_TEST(zeInitTests,
         GivenGPUFlagWhenInitializingGPUDriverThenSuccessIsReturned) {
  lzt::ze_init(ZE_INIT_FLAG_GPU_ONLY);
}

} // namespace
