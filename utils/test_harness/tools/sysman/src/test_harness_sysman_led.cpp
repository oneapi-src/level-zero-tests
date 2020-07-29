/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_led_handle_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumLeds(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_led_handle_t> get_led_handles(zes_device_handle_t device,
                                              uint32_t &count) {
  if (count == 0)
    count = get_led_handle_count(device);
  std::vector<zes_led_handle_t> ledHandles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumLeds(device, &count, ledHandles.data()));
  return ledHandles;
}

zes_led_properties_t get_led_properties(zes_led_handle_t ledHandle) {
  zes_led_properties_t properties = {ZES_STRUCTURE_TYPE_LED_PROPERTIES,
                                     nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesLedGetProperties(ledHandle, &properties));
  return properties;
}
zes_led_state_t get_led_state(zes_led_handle_t ledHandle) {
  zes_led_state_t state = {ZES_STRUCTURE_TYPE_LED_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesLedGetState(ledHandle, &state));
  return state;
}
void set_led_state(zes_led_handle_t ledHandle, ze_bool_t enable) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesLedSetState(ledHandle, enable));
}
void set_led_color(zes_led_handle_t ledHandle, zes_led_color_t color) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesLedSetColor(ledHandle, &color));
}
} // namespace level_zero_tests
