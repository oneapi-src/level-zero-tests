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
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(ledHandles.size(), count);
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
    }
  }
}

TEST_F(
    LedModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_led_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    lzt::get_led_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    LedModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarLedHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ledHandlesInitial = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandlesInitial) {
      ASSERT_NE(nullptr, ledHandle);
    }

    count = 0;
    auto ledHandlesLater = lzt::get_led_handles(device, count);
    for (auto ledHandle : ledHandlesLater) {
      ASSERT_NE(nullptr, ledHandle);
    }
    EXPECT_EQ(ledHandlesInitial, ledHandlesLater);
  }
}

TEST_F(
    LedModuleTest,
    GivenValidLedHandleWhenRetrievingLedPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto properties = lzt::get_led_properties(ledHandle);
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
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto propertiesInitial = lzt::get_led_properties(ledHandle);
      auto propertiesLater = lzt::get_led_properties(ledHandle);
      EXPECT_EQ(propertiesInitial.canControl, propertiesLater.canControl);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(propertiesInitial.haveRGB, propertiesLater.haveRGB);
    }
  }
}
TEST_F(LedModuleTest,
       GivenValidLedHandleWhenRetrievingLedStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto properties = lzt::get_led_properties(ledHandle);
      auto state = lzt::get_led_state(ledHandle);
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
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto initial_state = lzt::get_led_state(ledHandle);
      if (initial_state.isOn == false) {
        lzt::set_led_state(ledHandle, true);
      }
      zes_led_color_t color = {};
      color.red = 1.0;
      color.blue = 1.0;
      color.green = 1.0;
      lzt::set_led_color(ledHandle, color);
      zes_led_state_t get_state = lzt::get_led_state(ledHandle);
      EXPECT_EQ(get_state.isOn, true);
      EXPECT_DOUBLE_EQ(get_state.color.red, 1.0);
      EXPECT_DOUBLE_EQ(get_state.color.blue, 1.0);
      EXPECT_DOUBLE_EQ(get_state.color.green, 1.0);
      color.red = initial_state.color.red;
      color.blue = initial_state.color.blue;
      color.green = initial_state.color.green;
      lzt::set_led_color(ledHandle, color);
    }
  }
}
TEST_F(LedModuleTest,
       GivenValidLedHandleWhenSettingLedStateToOffThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      lzt::set_led_state(ledHandle, false);
      auto get_state = lzt::get_led_state(ledHandle);
      EXPECT_EQ(get_state.isOn, false);
      EXPECT_DOUBLE_EQ(get_state.color.red, 0.0);
      EXPECT_DOUBLE_EQ(get_state.color.blue, 0.0);
      EXPECT_DOUBLE_EQ(get_state.color.green, 0.0);
      lzt::set_led_state(ledHandle, true);
    }
  }
}

} // namespace
