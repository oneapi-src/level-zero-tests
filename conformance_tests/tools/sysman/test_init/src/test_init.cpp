/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

TEST(zetSysmanGetTests,
     GivenValidDeviceHandleWhenRetrievingSysmanHandleThenSuccessIsReturned) {
  auto devices = lzt::get_ze_devices();
  for (auto device : devices) {
    lzt::get_sysman_handle(device);
  }
}

TEST(
    zetSysmanGetTests,
    GivenValidDeviceHandleWhenRetrievingSysmanHandleThenNotNullSysmanHandleReturned) {
  auto devices = lzt::get_ze_devices();
  for (auto device : devices) {
    zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
    EXPECT_NE(nullptr, hSysmanDevice);
  }
}

} // namespace
