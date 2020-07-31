/* Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_TEMP_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_TEMP_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_temp_handle_count(zes_device_handle_t device);

std::vector<zes_temp_handle_t> get_temp_handles(zes_device_handle_t device,
                                                uint32_t &count);

zes_temp_properties_t get_temp_properties(zes_temp_handle_t tempHandle);

zes_temp_config_t get_temp_config(zes_temp_handle_t tempHandle);

ze_result_t set_temp_config(zes_temp_handle_t tempHandle,
                            zes_temp_config_t &config);

double get_temp_state(zes_temp_handle_t tempHandle);

} // namespace level_zero_tests

#endif
