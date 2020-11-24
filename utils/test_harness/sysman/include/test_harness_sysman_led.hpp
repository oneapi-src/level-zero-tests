/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_LED_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_LED_HPP

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_led_handle_count(zes_device_handle_t device);

std::vector<zes_led_handle_t> get_led_handles(zes_device_handle_t device,
                                              uint32_t &count);

zes_led_properties_t get_led_properties(zes_led_handle_t ledHandle);

zes_led_state_t get_led_state(zes_led_handle_t ledHandle);

void set_led_state(zes_led_handle_t ledHandle, ze_bool_t enable);

void set_led_color(zes_led_handle_t ledHandle, zes_led_color_t color);

} // namespace level_zero_tests

#endif
