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

TEST(
    zeInitTests,
    GivenDriverWasAlreadyInitializedWhenInitializingDriverThenSuccessIsReturned) {
  for (int i = 0; i < 5; ++i) {
    lzt::ze_init();
  }
}

TEST(zeDriverGetDriverVersionTests,
     GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned) {

  lzt::ze_init();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    uint32_t version = lzt::get_driver_version(driver);
    LOG_INFO << "Driver version: " << version;
  }
}

TEST(zeDriverGetApiVersionTests,
     GivenValidDriverWhenRetrievingApiVersionThenValidApiVersionIsReturned) {
  lzt::ze_init();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    ze_api_version_t api_version = lzt::get_api_version(driver);
    LOG_INFO << "API version: " << api_version;
  }
}

TEST(zeDriverGetIPCPropertiesTests,
     GivenValidDriverWhenRetrievingIPCPropertiesThenValidPropertiesAreRetured) {
  lzt::ze_init();
  auto drivers = lzt::get_all_driver_handles();
  ASSERT_GT(drivers.size(), 0);
  for (auto driver : drivers) {
    lzt::get_ipc_properties(driver);
  }
}

} // namespace
