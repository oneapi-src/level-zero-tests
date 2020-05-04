/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_led_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanLedGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_led_handle_t> get_led_handles(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0)
    count = get_led_handle_count(device);
  std::vector<zet_sysman_led_handle_t> ledHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanLedGet(hSysman, &count, ledHandles.data()));
  return ledHandles;
}

zet_led_properties_t get_led_properties(zet_sysman_led_handle_t ledHandle) {
  zet_led_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanLedGetProperties(ledHandle, &properties));
  return properties;
}
zet_led_state_t get_led_state(zet_sysman_led_handle_t ledHandle) {
  zet_led_state_t state;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanLedGetState(ledHandle, &state));
  return state;
}
void set_led_state(zet_sysman_led_handle_t ledHandle, zet_led_state_t state) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanLedSetState(ledHandle, &state));
}
} // namespace level_zero_tests
