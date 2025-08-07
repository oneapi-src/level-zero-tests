/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

#ifdef USE_ZESINIT
class LedModuleZesTest : public lzt::ZesSysmanCtsClass {
public:
  bool is_led_supported = false;
};
#define LED_TEST LedModuleZesTest
#else // USE_ZESINIT
class LedModuleTest : public lzt::SysmanCtsClass {
public:
  bool is_led_supported = false;
};
#define LED_TEST LedModuleTest
#endif // USE_ZESINIT

LZT_TEST_F(
    LED_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_led_handle_count(device);
  }
}

LZT_TEST_F(
    LED_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullLedHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
      ASSERT_EQ(led_handles.size(), count);
      for (auto led_handle : led_handles) {
        ASSERT_NE(nullptr, led_handle);
      }
    } else {
      LOG_INFO << "No led handles found for this device!";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    LED_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    actual_count = lzt::get_led_handle_count(device);
    if (actual_count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      lzt::get_led_handles(device, actual_count);
      uint32_t test_count = actual_count + 1;
      lzt::get_led_handles(device, test_count);
      EXPECT_EQ(test_count, actual_count);
    } else {
      LOG_INFO << "No led handles found for this device!";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    LED_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarLedHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles_initial = lzt::get_led_handles(device, count);
      for (auto led_handle : led_handles_initial) {
        ASSERT_NE(nullptr, led_handle);
      }
      count = 0;
      auto led_handles_later = lzt::get_led_handles(device, count);
      for (auto led_handle : led_handles_later) {
        ASSERT_NE(nullptr, led_handle);
      }
      EXPECT_EQ(led_handles_initial, led_handles_later);
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    LED_TEST,
    GivenValidLedHandleWhenRetrievingLedPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
      for (auto led_handle : led_handles) {
        ASSERT_NE(nullptr, led_handle);
        auto properties = lzt::get_led_properties(led_handle);
        if (properties.onSubdevice) {
          EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
        }
      }
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    LED_TEST,
    GivenValidLedHandleWhenRetrievingLedPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
      for (auto led_handle : led_handles) {
        ASSERT_NE(nullptr, led_handle);
        auto properties_initial = lzt::get_led_properties(led_handle);
        auto properties_later = lzt::get_led_properties(led_handle);
        EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
        if (properties_initial.onSubdevice && properties_later.onSubdevice) {
          EXPECT_EQ(properties_initial.subdeviceId,
                    properties_later.subdeviceId);
        }
        EXPECT_EQ(properties_initial.haveRGB, properties_later.haveRGB);
      }
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}
LZT_TEST_F(LED_TEST,
           GivenValidLedHandleWhenRetrievingLedStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
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
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on any of the devices! ";
  }
}
LZT_TEST_F(LED_TEST,
           GivenValidLedHandleWhenSettingLedColorTheSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
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
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on aby of the devices! ";
  }
}
LZT_TEST_F(LED_TEST,
           GivenValidLedHandleWhenSettingLedStateToOffThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_led_handle_count(device);
    if (count > 0) {
      is_led_supported = true;
      LOG_INFO << "Led handles are available on this device!";
      auto led_handles = lzt::get_led_handles(device, count);
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
    } else {
      LOG_INFO << "No led handles found for this device! ";
    }
  }
  if (!is_led_supported) {
    FAIL() << "No led handles found on aby of the devices! ";
  }
}

} // namespace
