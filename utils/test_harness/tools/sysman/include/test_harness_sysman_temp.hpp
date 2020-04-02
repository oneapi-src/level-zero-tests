/* Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_TEMP_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_TEMP_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_temp_handle_count(ze_device_handle_t device);

std::vector<zet_sysman_temp_handle_t>
get_temp_handles(ze_device_handle_t device, uint32_t &count);

zet_temp_properties_t get_temp_properties(zet_sysman_temp_handle_t tempHandle);

zet_temp_config_t get_temp_config(zet_sysman_temp_handle_t tempHandle);

void set_temp_config(zet_sysman_temp_handle_t tempHandle,
                     zet_temp_config_t &config);

double get_temp_state(zet_sysman_temp_handle_t tempHandle);

} // namespace level_zero_tests

#endif
