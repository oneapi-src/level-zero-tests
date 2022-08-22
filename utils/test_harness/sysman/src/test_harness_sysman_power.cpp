/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {
uint32_t get_power_handle_count(zes_device_handle_t device) {
  uint32_t p_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumPowerDomains(device, &p_count, nullptr));
  EXPECT_GE(p_count, 0);
  return p_count;
}
std::vector<zes_pwr_handle_t> get_power_handles(zes_device_handle_t device,
                                                uint32_t &p_count) {
  if (p_count == 0)
    p_count = get_power_handle_count(device);
  std::vector<zes_pwr_handle_t> p_power_handles(p_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumPowerDomains(
                                   device, &p_count, p_power_handles.data()));
  return p_power_handles;
}
zes_power_properties_t get_power_properties(zes_pwr_handle_t pPowerHandle) {
  zes_power_properties_t pProperties = {ZES_STRUCTURE_TYPE_POWER_PROPERTIES,
                                        nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerGetProperties(pPowerHandle, &pProperties));
  return pProperties;
}
uint32_t get_power_limit_count(zes_pwr_handle_t hPower) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetLimitsExt(hPower, &pCount, nullptr));
  EXPECT_GE(pCount, 0);
  return pCount;
}
std::vector<zes_power_limit_ext_desc_t>
get_power_limits_ext(zes_pwr_handle_t hPower, uint32_t *pCount) {
  if (*pCount == 0)
    *pCount = get_power_limit_count(hPower);
  std::vector<zes_power_limit_ext_desc_t> p_power_limits_descriptors(*pCount);
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zesPowerGetLimitsExt(hPower, pCount, p_power_limits_descriptors.data()));
  return p_power_limits_descriptors;
}
void set_power_limits_ext(zes_pwr_handle_t hPower, uint32_t *pCount,
                          zes_power_limit_ext_desc_t *pSustained) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerSetLimitsExt(hPower, pCount, pSustained));
}
void compare_power_descriptor_structures(
    zes_power_limit_ext_desc_t firstDescriptor,
    zes_power_limit_ext_desc_t secondDescriptor) {

  EXPECT_EQ(firstDescriptor.level, secondDescriptor.level);
  EXPECT_EQ(firstDescriptor.source, secondDescriptor.source);
  EXPECT_EQ(firstDescriptor.limitUnit, secondDescriptor.limitUnit);
  EXPECT_EQ(firstDescriptor.enabledStateLocked,
            secondDescriptor.enabledStateLocked);
  EXPECT_EQ(firstDescriptor.enabled, secondDescriptor.enabled);
  EXPECT_EQ(firstDescriptor.intervalValueLocked,
            secondDescriptor.intervalValueLocked);
  EXPECT_EQ(firstDescriptor.interval, secondDescriptor.interval);
  EXPECT_EQ(firstDescriptor.limitValueLocked,
            secondDescriptor.limitValueLocked);
  EXPECT_EQ(firstDescriptor.limit, secondDescriptor.limit);
}
void get_power_limits(zes_pwr_handle_t pPowerHandle,
                      zes_power_sustained_limit_t *pSustained,
                      zes_power_burst_limit_t *pBurst,
                      zes_power_peak_limit_t *pPeak) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerGetLimits(pPowerHandle, pSustained, pBurst, pPeak));
}
void set_power_limits(zes_pwr_handle_t pPowerHandle,
                      zes_power_sustained_limit_t *pSustained,
                      zes_power_burst_limit_t *pBurst,
                      zes_power_peak_limit_t *pPeak) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerSetLimits(pPowerHandle, pSustained, pBurst, pPeak));
}
void get_power_energy_counter(zes_pwr_handle_t pPowerHandle,
                              zes_power_energy_counter_t *pEnergy) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPowerGetEnergyCounter(pPowerHandle, pEnergy));
}
zes_energy_threshold_t
get_power_energy_threshold(zes_pwr_handle_t pPowerHandle) {
  zes_energy_threshold_t pThreshold = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerGetEnergyThreshold(pPowerHandle, &pThreshold));
  return pThreshold;
}
void set_power_energy_threshold(zes_pwr_handle_t pPowerHandle,
                                double threshold) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesPowerSetEnergyThreshold(pPowerHandle, threshold));
}
} // namespace level_zero_tests
