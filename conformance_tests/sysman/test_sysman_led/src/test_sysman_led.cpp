/*
 *
 * Copyright (C) 2020 Intel Corporation
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
#include <level_zero/ze_api.h>

namespace {

class LedModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    LedModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_led_handle_count(device);
  }
}

TEST_F(
    LedModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullLedHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(led_handles.size(), count);
    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
    }
  }
}

TEST_F(
    LedModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_led_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_led_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    LedModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarLedHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles_initial = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles_initial) {
      ASSERT_NE(nullptr, led_handle);
    }

    count = 0;
    auto led_handles_later = lzt::get_led_handles(device, count);
    for (auto led_handle : led_handles_later) {
      ASSERT_NE(nullptr, led_handle);
    }
    EXPECT_EQ(led_handles_initial, led_handles_later);
  }
}

TEST_F(
    LedModuleTest,
    GivenValidLedHandleWhenRetrievingLedPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
      auto properties = lzt::get_led_properties(led_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

TEST_F(
    LedModuleTest,
    GivenValidLedHandleWhenRetrievingLedPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
      auto properties_initial = lzt::get_led_properties(led_handle);
      auto properties_later = lzt::get_led_properties(led_handle);
      EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.haveRGB, properties_later.haveRGB);
    }
  }
}
TEST_F(LedModuleTest,
       GivenValidLedHandleWhenRetrievingLedStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
      auto properties = lzt::get_led_properties(led_handle);
      auto state = lzt::get_led_state(led_handle);
      if (state.isOn == false) {
        ASSERT_DOUBLE_EQ(state.color.red, 0.0);
        ASSERT_DOUBLE_EQ(state.color.green, 0.0);
        ASSERT_DOUBLE_EQ(state.color.blue, 0.0);
      } else {
        if ((state.color.red == 0.0) && (state.color.green == 0.0) &&
            (state.color.blue == 0.0))
          FAIL();
        else
          SUCCEED();
      }
    }
  }
}
TEST_F(LedModuleTest,
       GivenValidLedHandleWhenSettingLedColorTheSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
      auto initial_state = lzt::get_led_state(led_handle);
      if (initial_state.isOn == false) {
        lzt::set_led_state(led_handle, true);
      }
      zes_led_color_t color = {};
      color.red = 1.0;
      color.blue = 1.0;
      color.green = 1.0;
      lzt::set_led_color(led_handle, color);
      zes_led_state_t get_state = lzt::get_led_state(led_handle);
      EXPECT_EQ(get_state.isOn, true);
      EXPECT_DOUBLE_EQ(get_state.color.red, 1.0);
      EXPECT_DOUBLE_EQ(get_state.color.blue, 1.0);
      EXPECT_DOUBLE_EQ(get_state.color.green, 1.0);
      color.red = initial_state.color.red;
      color.blue = initial_state.color.blue;
      color.green = initial_state.color.green;
      lzt::set_led_color(led_handle, color);
    }
  }
}
TEST_F(LedModuleTest,
       GivenValidLedHandleWhenSettingLedStateToOffThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto led_handles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto led_handle : led_handles) {
      ASSERT_NE(nullptr, led_handle);
      lzt::set_led_state(led_handle, false);
      auto get_state = lzt::get_led_state(led_handle);
      EXPECT_EQ(get_state.isOn, false);
      EXPECT_DOUBLE_EQ(get_state.color.red, 0.0);
      EXPECT_DOUBLE_EQ(get_state.color.blue, 0.0);
      EXPECT_DOUBLE_EQ(get_state.color.green, 0.0);
      lzt::set_led_state(led_handle, true);
    }
  }
}

} // namespace
