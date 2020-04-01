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

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_freq_handle_count(ze_device_handle_t device) {
  uint32_t pCount = 0;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGet(hSysmanDevice, &pCount, nullptr));
  EXPECT_GT(pCount, 0);
  return pCount;
}

std::vector<zet_sysman_freq_handle_t>
get_freq_handles(ze_device_handle_t device, uint32_t &pCount) {
  if (pCount == 0)
    pCount = get_freq_handle_count(device);
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  std::vector<zet_sysman_freq_handle_t> pFreqHandles(pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGet(hSysmanDevice, &pCount, pFreqHandles.data()));
  return pFreqHandles;
}

zet_freq_state_t get_freq_state(zet_sysman_freq_handle_t pFreqHandle) {
  zet_freq_state_t pState;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetState(pFreqHandle, &pState));
  return pState;
}

zet_freq_range_t get_freq_range(zet_sysman_freq_handle_t pFreqHandle) {
  zet_freq_range_t pLimits;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetRange(pFreqHandle, &pLimits));
  return pLimits;
}

void set_freq_range(zet_sysman_freq_handle_t pFreqHandle,
                    zet_freq_range_t &pLimits) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetRange(pFreqHandle, &pLimits));
}

zet_freq_properties_t
get_freq_properties(zet_sysman_freq_handle_t pFreqHandle) {
  zet_freq_properties_t pProperties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetProperties(pFreqHandle, &pProperties));
  EXPECT_LE(pProperties.min, pProperties.max);
  return pProperties;
}

zet_freq_range_t
get_and_validate_freq_range(zet_sysman_freq_handle_t pFreqHandle) {
  zet_freq_range_t pLimits = get_freq_range(pFreqHandle);
  zet_freq_properties_t pProperty = get_freq_properties(pFreqHandle);
  EXPECT_LE(pLimits.min, pLimits.max);
  EXPECT_GE(pLimits.min, pProperty.min);
  EXPECT_LE(pLimits.min, pProperty.max);
  EXPECT_GE(pLimits.max, pProperty.min);
  EXPECT_LE(pLimits.max, pProperty.max);
  return pLimits;
}

uint32_t get_available_clock_count(zet_sysman_freq_handle_t pFreqHandle) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetAvailableClocks(pFreqHandle, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<double> get_available_clocks(zet_sysman_freq_handle_t pFreqHandle,
                                         uint32_t &count) {
  if (count == 0)
    count = get_available_clock_count(pFreqHandle);
  std::vector<double> frequency(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFrequencyGetAvailableClocks(
                                   pFreqHandle, &count, frequency.data()));
  return frequency;
}

void validate_freq_state(zet_sysman_freq_handle_t pFreqHandle,
                         zet_freq_state_t pState) {
  zet_freq_range_t pLimits = get_and_validate_freq_range(pFreqHandle);
  zet_freq_properties_t pProperty = get_freq_properties(pFreqHandle);
  EXPECT_GE(pState.actual, pProperty.min);
  EXPECT_LE(pState.actual, pProperty.max);
  EXPECT_LE(pState.actual, pLimits.max);
  if (pState.throttleReasons == ZET_FREQ_THROTTLE_REASONS_NONE) {
    EXPECT_DOUBLE_EQ(pState.actual,
                     std::min<double>(pProperty.max, pState.request));
    EXPECT_DOUBLE_EQ(pState.tdp, pProperty.max);
  } else {
    EXPECT_LT(pState.actual, pState.request);
    EXPECT_LT(pState.tdp, pProperty.max);
  }

  EXPECT_GE(pState.request, pProperty.min);
  EXPECT_LE(pState.request, std::numeric_limits<std::uint32_t>::max());
  EXPECT_GE(pState.efficient, pProperty.min);
  EXPECT_LE(pState.efficient, pProperty.max);
}

void idle_check(zet_sysman_freq_handle_t pFreqHandle) {
  int wait = 0;

  /* Monitor frequencies until cur settles down to min, which should
   * happen within the allotted time */
  do {
    zet_freq_properties_t property = get_freq_properties(pFreqHandle);
    zet_freq_state_t state = get_freq_state(pFreqHandle);
    validate_freq_state(pFreqHandle, state);
    if (state.actual == property.min)
      break;
    std::this_thread::sleep_for(
        std::chrono::microseconds(1000 * IDLE_WAIT_TIMESTEP_MSEC));
    wait += IDLE_WAIT_TIMESTEP_MSEC;
  } while (wait < IDLE_WAIT_TIMEOUT_MSEC);
}
bool check_for_throttling(zet_sysman_freq_handle_t pFreqHandle) {
  int wait = 0;
  do {
    zet_freq_state_t state = get_freq_state(pFreqHandle);
    if (state.throttleReasons != ZET_FREQ_THROTTLE_REASONS_NONE)
      return true;
    std::this_thread::sleep_for(std::chrono::microseconds(1000 * 10));
    wait += 10;
  } while (wait < (IDLE_WAIT_TIMEOUT_MSEC * 2));
  return false;
}
zet_freq_throttle_time_t
get_throttle_time(zet_sysman_freq_handle_t pFreqHandle) {
  zet_freq_throttle_time_t pThrottletime;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyGetThrottleTime(pFreqHandle, &pThrottletime));
  return pThrottletime;
}

} // namespace level_zero_tests
