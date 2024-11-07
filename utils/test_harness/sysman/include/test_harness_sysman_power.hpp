/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_POWER_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_POWER_HPP
#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {
uint32_t get_power_handle_count(zes_device_handle_t device);
std::vector<zes_pwr_handle_t> get_power_handles(zes_device_handle_t device,
                                                uint32_t &count);
zes_pwr_handle_t get_card_power_handle(zes_device_handle_t device);
zes_power_properties_t get_power_properties(zes_pwr_handle_t pPowerhandle);
ze_result_t get_power_limits(zes_pwr_handle_t pPowerHandle,
                      zes_power_sustained_limit_t *pSustained,
                      zes_power_burst_limit_t *pBurst,
                      zes_power_peak_limit_t *pPeak);
ze_result_t set_power_limits(zes_pwr_handle_t pPowerHandle,
                      zes_power_sustained_limit_t *pSustained,
                      zes_power_burst_limit_t *pBurst,
                      zes_power_peak_limit_t *pPeak);
void get_power_energy_counter(zes_pwr_handle_t pPowerHandle,
                              zes_power_energy_counter_t *pEnergy);
ze_result_t get_power_energy_threshold(zes_pwr_handle_t pPowerHandle,
                                       zes_energy_threshold_t *pThreshold);
void set_power_energy_threshold(zes_pwr_handle_t pPowerHandle,
                                double threshold);
ze_result_t get_power_limits_ext(zes_pwr_handle_t hPower, uint32_t *pCount,
    std::vector<zes_power_limit_ext_desc_t> &pPowerLimitsDescriptors);
ze_result_t set_power_limits_ext(zes_pwr_handle_t hPower, uint32_t *pCount,
                          zes_power_limit_ext_desc_t *pSustained);
void compare_power_descriptor_structures(
    zes_power_limit_ext_desc_t firstDescriptor,
    zes_power_limit_ext_desc_t secondDescriptor);
ze_result_t get_power_limit_count(zes_pwr_handle_t hPower, uint32_t *pCount);
} // namespace level_zero_tests

#endif
