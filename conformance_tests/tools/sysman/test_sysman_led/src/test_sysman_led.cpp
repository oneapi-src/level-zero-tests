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
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto properties = lzt::get_led_properties(ledHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
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
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto propertiesInitial = lzt::get_led_properties(ledHandle);
      auto propertiesLater = lzt::get_led_properties(ledHandle);
      EXPECT_EQ(propertiesInitial.canControl, propertiesLater.canControl);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
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
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto properties = lzt::get_led_properties(ledHandle);
      auto state = lzt::get_led_state(ledHandle);
      if (state.isOn == false) {
        ASSERT_EQ(state.red, 0);
        ASSERT_EQ(state.green, 0);
        ASSERT_EQ(state.blue, 0);
      } else {
        if ((state.red == 0) && (state.green == 0) && (state.blue == 0))
          FAIL();
        else
          SUCCEED();
      }
    }
  }
}
TEST_F(
    LedModuleTest,
    GivenValidLedHandleWhenSettingLedStateThenExpectzetSysmanLedSetStateFollowedByzetSysmanLedGetStateToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto initial_state = lzt::get_led_state(ledHandle);
      zet_led_state_t set_state;
      set_state.isOn = true;
      set_state.red = 255;
      set_state.blue = 255;
      set_state.green = 255;
      lzt::set_led_state(ledHandle, set_state);
      zet_led_state_t get_state = lzt::get_led_state(ledHandle);
      EXPECT_EQ(get_state.isOn, set_state.isOn);
      EXPECT_EQ(get_state.red, set_state.red);
      EXPECT_EQ(get_state.blue, set_state.blue);
      EXPECT_EQ(get_state.green, set_state.green);
      lzt::set_led_state(ledHandle, initial_state);
    }
  }
}
TEST_F(
    LedModuleTest,
    GivenValidLedHandleWhenSettingLedStateToOffThenExpectzetSysmanLedSetStateFollowedByzetSysmanLedGetStateToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ledHandles = lzt::get_led_handles(device, count);
    for (auto ledHandle : ledHandles) {
      ASSERT_NE(nullptr, ledHandle);
      auto properties = lzt::get_led_properties(ledHandle);
      auto initial_state = lzt::get_led_state(ledHandle);
      zet_led_state_t set_state;
      set_state.isOn = false;
      lzt::set_led_state(ledHandle, set_state);
      auto get_state = lzt::get_led_state(ledHandle);
      EXPECT_EQ(get_state.isOn, set_state.isOn);
      EXPECT_EQ(get_state.red, 0);
      EXPECT_EQ(get_state.blue, 0);
      EXPECT_EQ(get_state.green, 0);
      lzt::set_led_state(ledHandle, initial_state);
    }
  }
}

} // namespace
