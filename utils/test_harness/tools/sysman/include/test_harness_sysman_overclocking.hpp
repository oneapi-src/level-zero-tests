/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_OVERCLOCKING_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_OVERCLOCKING_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

zet_oc_config_t get_oc_config(zet_sysman_freq_handle_t freqHandle);

zet_oc_capabilities_t get_oc_capabilities(zet_sysman_freq_handle_t freqHandle);

void set_oc_config(zet_sysman_freq_handle_t freqHandle, zet_oc_config_t &config,
                   ze_bool_t *restart);

double get_oc_iccmax(zet_sysman_freq_handle_t freqHandle);
void set_oc_iccmax(zet_sysman_freq_handle_t freqHandle, double icmax);
double get_oc_tjmax(zet_sysman_freq_handle_t freqHandle);
void set_oc_tjmax(zet_sysman_freq_handle_t freqHandle, double tjmax);

} // namespace level_zero_tests

#endif
