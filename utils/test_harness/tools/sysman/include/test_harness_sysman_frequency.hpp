/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FREQUENCY_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FREQUENCY_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

#define IDLE_WAIT_TIMESTEP_MSEC 250
#define IDLE_WAIT_TIMEOUT_MSEC 2500
namespace level_zero_tests {

void idle_check(zet_sysman_freq_handle_t pFreqHandle);
void validate_freq_state(zet_sysman_freq_handle_t pFreqHandle,
                         zet_freq_state_t pState);
uint32_t get_freq_handle_count(ze_device_handle_t device);
std::vector<zet_sysman_freq_handle_t>
get_freq_handles(ze_device_handle_t device, uint32_t &count);
zet_freq_state_t get_freq_state(zet_sysman_freq_handle_t pFreqHandle);
zet_freq_range_t get_freq_range(zet_sysman_freq_handle_t pFreqHandle);
void set_freq_range(zet_sysman_freq_handle_t pFreqHandle,
                    zet_freq_range_t &pLimits);
zet_freq_properties_t get_freq_properties(zet_sysman_freq_handle_t pFreqHandle);
zet_freq_range_t
get_and_validate_freq_range(zet_sysman_freq_handle_t pFreqHandle);
uint32_t get_available_clock_count(zet_sysman_freq_handle_t pFreqHandle);
std::vector<double> get_available_clocks(zet_sysman_freq_handle_t pFreqHandle,
                                         uint32_t &count);
bool check_for_throttling(zet_sysman_freq_handle_t pFreqHandle);
zet_freq_throttle_time_t
get_throttle_time(zet_sysman_freq_handle_t pFreqHandle);

} // namespace level_zero_tests

#endif
