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

std::vector<zet_sysman_fan_handle_t> get_fan_handles(ze_device_handle_t device,
                                                     uint32_t &count);

zet_fan_properties_t get_fan_properties(zet_sysman_fan_handle_t fanHandle);

zet_fan_config_t get_fan_configuration(zet_sysman_fan_handle_t fanHandle);

void set_fan_configuration(zet_sysman_fan_handle_t fanHandle,
                           zet_fan_config_t &config);

void get_fan_state(zet_sysman_fan_handle_t fanHandle,
                   zet_fan_speed_units_t units, uint32_t &speed);

} // namespace level_zero_tests

#endif
