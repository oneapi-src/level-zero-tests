/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {
uint32_t get_power_handle_count(ze_device_handle_t device) {
  uint32_t pCount = 0;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGet(hSysmanDevice, &pCount, nullptr));
  EXPECT_GT(pCount, 0);
  return pCount;
}
std::vector<zet_sysman_pwr_handle_t>
get_power_handles(ze_device_handle_t device, uint32_t &pCount) {
  if (pCount == 0)
    pCount = get_power_handle_count(device);
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  std::vector<zet_sysman_pwr_handle_t> pPowerHandles(pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGet(hSysmanDevice, &pCount, pPowerHandles.data()));
  return pPowerHandles;
}
zet_power_properties_t
get_power_properties(zet_sysman_pwr_handle_t pPowerHandle) {
  zet_power_properties_t pProperties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGetProperties(pPowerHandle, &pProperties));
  return pProperties;
}
void get_power_limits(zet_sysman_pwr_handle_t pPowerHandle,
                      zet_power_sustained_limit_t *pSustained,
                      zet_power_burst_limit_t *pBurst,
                      zet_power_peak_limit_t *pPeak) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGetLimits(pPowerHandle, pSustained, pBurst, pPeak));
}
void set_power_limits(zet_sysman_pwr_handle_t pPowerHandle,
                      zet_power_sustained_limit_t *pSustained,
                      zet_power_burst_limit_t *pBurst,
                      zet_power_peak_limit_t *pPeak) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerSetLimits(pPowerHandle, pSustained, pBurst, pPeak));
}
void get_power_energy_counter(zet_sysman_pwr_handle_t pPowerHandle,
                              zet_power_energy_counter_t *pEnergy) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGetEnergyCounter(pPowerHandle, pEnergy));
}
zet_energy_threshold_t
get_power_energy_threshold(zet_sysman_pwr_handle_t pPowerHandle) {
  zet_energy_threshold_t pThreshold;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerGetEnergyThreshold(pPowerHandle, &pThreshold));
  return pThreshold;
}
void set_power_energy_threshold(zet_sysman_pwr_handle_t pPowerHandle,
                                double threshold) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPowerSetEnergyThreshold(pPowerHandle, threshold));
}
} // namespace level_zero_tests
