/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FAN_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FAN_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_fan_handle_count(ze_device_handle_t device);

std::vector<zes_fan_handle_t> get_fan_handles(ze_device_handle_t device,
                                              uint32_t &count);

zes_fan_properties_t get_fan_properties(zes_fan_handle_t fanHandle);

zes_fan_config_t get_fan_configuration(zes_fan_handle_t fanHandle);

void get_fan_state(zes_fan_handle_t fanHandle, zes_fan_speed_units_t units,
                   int32_t &speed);

void set_fan_default_mode(zes_fan_handle_t fanHandle);

void set_fan_fixed_speed_mode(zes_fan_handle_t fanHandle,
                              zes_fan_speed_t &speed);

void set_fan_speed_table_mode(zes_fan_handle_t fanHandle,
                              zes_fan_speed_table_t &speedTable);
} // namespace level_zero_tests

#endif
