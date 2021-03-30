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
#include "logging/logging.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_freq_handle_count(zes_device_handle_t device) {
  uint32_t p_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFrequencyDomains(device, &p_count, nullptr));
  EXPECT_GE(p_count, 0);
  return p_count;
}

std::vector<zes_freq_handle_t> get_freq_handles(zes_device_handle_t device,
                                                uint32_t &p_count) {
  if (p_count == 0)
    p_count = get_freq_handle_count(device);
  std::vector<zes_freq_handle_t> pFreqHandles(p_count);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFrequencyDomains(
                                   device, &p_count, pFreqHandles.data()));
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
  if (pLimits.min > 0) {
    EXPECT_LE(pLimits.min, pLimits.max);
    EXPECT_GE(pLimits.min, pProperty.min);
  } else {
    LOG_INFO << "no external minimum frequency limit is in effect";
  }
  if (pLimits.max > 0) {
    EXPECT_LE(pLimits.min, pProperty.max);
    EXPECT_GE(pLimits.max, pProperty.min);
    EXPECT_LE(pLimits.max, pProperty.max);
  } else {
    LOG_INFO << "no external maximum frequency limit is in effect";
  }
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
  if (pState.actual > 0) {
    EXPECT_LT(pState.actual, DBL_MAX);
  } else {
    LOG_INFO << "Actual frequency is unknown";
  }
  if (pState.request > 0) {
    EXPECT_LT(pState.request, DBL_MAX);
  } else {
    LOG_INFO << "Requested frequency is unknown";
  }
  if (pState.efficient > 0) {
    EXPECT_LT(pState.efficient, DBL_MAX);
  } else {
    LOG_INFO << "Efficient frequency is unknown";
  }
  if (pState.tdp > 0) {
    EXPECT_LT(pState.tdp, DBL_MAX);
  } else {
    LOG_INFO << "TDP frequency is unknown";
  }
  EXPECT_GE(pState.throttleReasons, 0);
  EXPECT_LE(pState.throttleReasons,
            ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE |
                ZES_FREQ_THROTTLE_REASON_FLAG_SW_RANGE |
                ZES_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT |
                ZES_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT |
                ZES_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT |
                ZES_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP |
                ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP);
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
        state.throttleReasons <= ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE |
            ZES_FREQ_THROTTLE_REASON_FLAG_SW_RANGE |
            ZES_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT |
            ZES_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT |
            ZES_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT |
            ZES_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP |
            ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP)
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
