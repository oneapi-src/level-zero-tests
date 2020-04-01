/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_POWER_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_POWER_HPP
#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {
uint32_t get_power_handle_count(ze_device_handle_t device);
std::vector<zet_sysman_pwr_handle_t>
get_power_handles(ze_device_handle_t device, uint32_t &count);
zet_power_properties_t
get_power_properties(zet_sysman_pwr_handle_t pPowerhandle);
void get_power_limits(zet_sysman_pwr_handle_t pPowerHandle,
                      zet_power_sustained_limit_t *pSustained,
                      zet_power_burst_limit_t *pBurst,
                      zet_power_peak_limit_t *pPeak);
void set_power_limits(zet_sysman_pwr_handle_t pPowerHandle,
                      zet_power_sustained_limit_t *pSustained,
                      zet_power_burst_limit_t *pBurst,
                      zet_power_peak_limit_t *pPeak);
void get_power_energy_counter(zet_sysman_pwr_handle_t pPowerHandle,
                              zet_power_energy_counter_t *pEnergy);
zet_energy_threshold_t
get_power_energy_threshold(zet_sysman_pwr_handle_t pPowerHandle);
void set_power_energy_threshold(zet_sysman_pwr_handle_t pPowerHandle,
                                double threshold);
} // namespace level_zero_tests

#endif
