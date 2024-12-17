/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FREQUENCY_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FREQUENCY_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

#define IDLE_WAIT_TIMESTEP_MSEC 250
#define IDLE_WAIT_TIMEOUT_MSEC 2500
namespace level_zero_tests {

void idle_check(zes_freq_handle_t pFreqHandle);
void validate_freq_state(zes_freq_handle_t pFreqHandle,
                         zes_freq_state_t pState);
uint32_t get_freq_handle_count(zes_device_handle_t device);
std::vector<zes_freq_handle_t> get_freq_handles(zes_device_handle_t device,
                                                uint32_t &count);
zes_freq_state_t get_freq_state(zes_freq_handle_t pFreqHandle);
zes_freq_range_t get_freq_range(zes_freq_handle_t pFreqHandle);
void set_freq_range(zes_freq_handle_t pFreqHandle, zes_freq_range_t &pLimits);
void set_freq_range(zes_freq_handle_t pFreqHandle,
                           zes_freq_range_t &pLimits, ze_result_t &result);
zes_freq_properties_t get_freq_properties(zes_freq_handle_t pFreqHandle);
zes_freq_range_t get_and_validate_freq_range(zes_freq_handle_t pFreqHandle);
uint32_t get_available_clock_count(zes_freq_handle_t pFreqHandle);
std::vector<double> get_available_clocks(zes_freq_handle_t pFreqHandle,
                                         uint32_t &count);
bool check_for_throttling(zes_freq_handle_t pFreqHandle);
zes_freq_throttle_time_t get_throttle_time(zes_freq_handle_t pFreqHandle);

} // namespace level_zero_tests

#endif
