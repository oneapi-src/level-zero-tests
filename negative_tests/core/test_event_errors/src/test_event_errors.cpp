/*
 *
 * Copyright (C) 2026 Intel Corporation
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

class EventGetCounterBasedFlagsNegativeTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    if (lzt::get_api_version(lzt::get_default_driver()) < ZE_API_VERSION_1_17) {
      GTEST_SKIP() << "zeEventGetCounterBasedFlags requires API version 1.17";
    }
    ASSERT_NE(nullptr, get_counter_based_flags_)
        << "Driver supports API version 1.17 but the loader does not export "
           "zeEventGetCounterBasedFlags";
  }

  const decltype(&zeEventGetCounterBasedFlags) get_counter_based_flags_ =
      LZT_LOADER_FUNCTION(zeEventGetCounterBasedFlags);
};

LZT_TEST_F(
    EventGetCounterBasedFlagsNegativeTests,
    GivenInvalidEventHandleWhenGettingCounterBasedFlagsThenInvalidNullHandleErrorIsReturned) {
  ze_event_counter_based_flags_t flags = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            get_counter_based_flags_(nullptr, &flags));
}

LZT_TEST_F(
    EventGetCounterBasedFlagsNegativeTests,
    GivenInvalidFlagsPointerWhenGettingCounterBasedFlagsThenInvalidNullPointerErrorIsReturned) {
  ep.InitEventPool(1);
  ze_event_handle_t event = nullptr;
  ep.create_event(event);
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
            get_counter_based_flags_(event, nullptr));
  ep.destroy_event(event);
}

} // namespace
