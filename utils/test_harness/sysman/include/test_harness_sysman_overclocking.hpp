/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_OVERCLOCKING_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_OVERCLOCKING_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

zes_oc_capabilities_t get_oc_capabilities(zes_freq_handle_t freqHandle);

double get_oc_freq_target(zes_freq_handle_t freqHandle);

void set_oc_freq_target(zes_freq_handle_t freqHandle, double frequency);

double get_oc_voltage_target(zes_freq_handle_t freqHandle);

double get_oc_voltage_offset(zes_freq_handle_t freqHandle);

void set_oc_voltage(zes_freq_handle_t freqHandle, double target, double offset);

zes_oc_mode_t get_oc_mode(zes_freq_handle_t freqHandle);

void set_oc_mode(zes_freq_handle_t freqHandle, zes_oc_mode_t mode);

double get_oc_iccmax(zes_freq_handle_t freqHandle);

void set_oc_iccmax(zes_freq_handle_t freqHandle, double icmax);

double get_oc_tjmax(zes_freq_handle_t freqHandle);

void set_oc_tjmax(zes_freq_handle_t freqHandle, double tjmax);

} // namespace level_zero_tests

#endif
