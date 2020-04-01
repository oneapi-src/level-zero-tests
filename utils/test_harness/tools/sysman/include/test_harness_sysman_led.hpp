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
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_led_handle_count(ze_device_handle_t device);

std::vector<zet_sysman_led_handle_t> get_led_handles(ze_device_handle_t device,
                                                     uint32_t &count);

zet_led_properties_t get_led_properties(zet_sysman_led_handle_t ledHandle);

zet_led_state_t get_led_state(zet_sysman_led_handle_t ledHandle);

void set_led_state(zet_sysman_led_handle_t ledHandle, zet_led_state_t state);

} // namespace level_zero_tests

#endif
