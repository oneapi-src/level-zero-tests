/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_freq_handle_count(zes_device_handle_t device) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(device, &pCount, nullptr));
  EXPECT_GE(pCount, 0);
  return pCount;
}

std::vector<zes_freq_handle_t> get_freq_handles(zes_device_handle_t device,
                                                uint32_t &pCount) {
  if (pCount == 0)
    pCount = get_freq_handle_count(device);
  std::vector<zes_freq_handle_t> pFreqHandles(pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   device, &pCount, pFreqHandles.data()));
  return pFreqHandles;
}

zes_freq_state_t get_freq_state(zes_freq_handle_t pFreqHandle) {
  zes_freq_state_t pState = {ZES_STRUCTURE_TYPE_FREQ_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyGetState(pFreqHandle, &pState));
  return pState;
}

zes_freq_range_t get_freq_range(zes_freq_handle_t pFreqHandle) {
  zes_freq_range_t pLimits = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyGetRange(pFreqHandle, &pLimits));
  return pLimits;
}

void set_freq_range(zes_freq_handle_t pFreqHandle, zes_freq_range_t &pLimits) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencySetRange(pFreqHandle, &pLimits));
}

zes_freq_properties_t get_freq_properties(zes_freq_handle_t pFreqHandle) {
  zes_freq_properties_t pProperties = {ZES_STRUCTURE_TYPE_FREQ_PROPERTIES,
                                       nullptr};
  zesFrequencyGetProperties(pFreqHandle, &pProperties);
  EXPECT_LE(pProperties.min, pProperties.max);
  return pProperties;
}

zes_freq_range_t get_and_validate_freq_range(zes_freq_handle_t pFreqHandle) {
  zes_freq_range_t pLimits = get_freq_range(pFreqHandle);
  zes_freq_properties_t pProperty = get_freq_properties(pFreqHandle);
  EXPECT_LE(pLimits.min, pLimits.max);
  EXPECT_GE(pLimits.min, pProperty.min);
  EXPECT_LE(pLimits.min, pProperty.max);
  EXPECT_GE(pLimits.max, pProperty.min);
  EXPECT_LE(pLimits.max, pProperty.max);
  return pLimits;
}

uint32_t get_available_clock_count(zes_freq_handle_t pFreqHandle) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyGetAvailableClocks(pFreqHandle, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<double> get_available_clocks(zes_freq_handle_t pFreqHandle,
                                         uint32_t &count) {
  if (count == 0)
    count = get_available_clock_count(pFreqHandle);
  std::vector<double> frequency(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyGetAvailableClocks(
                                   pFreqHandle, &count, frequency.data()));
  return frequency;
}

void validate_freq_state(zes_freq_handle_t pFreqHandle,
                         zes_freq_state_t pState) {
  zes_freq_range_t pLimits = get_and_validate_freq_range(pFreqHandle);
  zes_freq_properties_t pProperty = get_freq_properties(pFreqHandle);
  zes_oc_capabilities_t ocProperty = get_oc_capabilities(pFreqHandle);
  EXPECT_GE(pState.actual, pProperty.min);
  EXPECT_LE(pState.actual, pProperty.max);
  EXPECT_LE(pState.actual, pLimits.max);
  if (!(pState.throttleReasons >= ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP &&
        pState.throttleReasons <= ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE)) {
    EXPECT_DOUBLE_EQ(pState.actual,
                     std::min<double>(pProperty.max, pState.request));
    EXPECT_DOUBLE_EQ(pState.tdp, pProperty.max);
  } else {
    EXPECT_LT(pState.actual, pState.request);
    EXPECT_LT(pState.tdp, pProperty.max);
  }

  EXPECT_GE(pState.request, pProperty.min);
  EXPECT_LE(pState.currentVoltage, ocProperty.maxOcVoltage);
  EXPECT_LE(pState.request, std::numeric_limits<std::uint32_t>::max());
  EXPECT_GE(pState.efficient, pProperty.min);
  EXPECT_LE(pState.efficient, pProperty.max);
}

void idle_check(zes_freq_handle_t pFreqHandle) {
  int wait = 0;

  /* Monitor frequencies until cur settles down to min, which should
   * happen within the allotted time */
  do {
    zes_freq_properties_t property = get_freq_properties(pFreqHandle);
    zes_freq_state_t state = get_freq_state(pFreqHandle);
    validate_freq_state(pFreqHandle, state);
    if (state.actual == property.min)
      break;
    std::this_thread::sleep_for(
        std::chrono::microseconds(1000 * IDLE_WAIT_TIMESTEP_MSEC));
    wait += IDLE_WAIT_TIMESTEP_MSEC;
  } while (wait < IDLE_WAIT_TIMEOUT_MSEC);
}
bool check_for_throttling(zes_freq_handle_t pFreqHandle) {
  int wait = 0;
  do {
    zes_freq_state_t state = get_freq_state(pFreqHandle);
    if (state.throttleReasons >= ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP &&
        state.throttleReasons <= ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE)
      return true;
    std::this_thread::sleep_for(std::chrono::microseconds(1000 * 10));
    wait += 10;
  } while (wait < (IDLE_WAIT_TIMEOUT_MSEC * 2));
  return false;
}
zes_freq_throttle_time_t get_throttle_time(zes_freq_handle_t pFreqHandle) {
  zes_freq_throttle_time_t pThrottletime = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyGetThrottleTime(pFreqHandle, &pThrottletime));
  return pThrottletime;
}

} // namespace level_zero_tests
