/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zes_oc_capabilities_t get_oc_capabilities(zes_freq_handle_t freqHandle) {
  zes_oc_capabilities_t capabilities = {ZES_STRUCTURE_TYPE_OC_CAPABILITIES,
                                        nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcGetCapabilities(freqHandle, &capabilities));
  return capabilities;
}

double get_oc_freq_target(zes_freq_handle_t freqHandle) {
  double target = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcGetFrequencyTarget(freqHandle, &target));
  return target;
}

void set_oc_freq_target(zes_freq_handle_t freqHandle, double target) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcSetFrequencyTarget(freqHandle, target));
}

double get_oc_voltage_target(zes_freq_handle_t freqHandle) {
  double target = 0;
  double offset = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcGetVoltageTarget(freqHandle, &target, &offset));
  return target;
}

double get_oc_voltage_offset(zes_freq_handle_t freqHandle) {
  double target = 0;
  double offset = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcGetVoltageTarget(freqHandle, &target, &offset));
  return offset;
}

void set_oc_voltage(zes_freq_handle_t freqHandle, double target,
                    double offset) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFrequencyOcGetVoltageTarget(freqHandle, &target, &offset));
}

zes_oc_mode_t get_oc_mode(zes_freq_handle_t freqHandle) {
  zes_oc_mode_t mode = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcGetMode(freqHandle, &mode));
  return mode;
}

void set_oc_mode(zes_freq_handle_t freqHandle, zes_oc_mode_t mode) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcSetMode(freqHandle, mode));
}

double get_oc_iccmax(zes_freq_handle_t freqHandle) {
  double icmax = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcGetIccMax(freqHandle, &icmax));
  return icmax;
}
void set_oc_iccmax(zes_freq_handle_t freqHandle, double icmax) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcSetIccMax(freqHandle, icmax));
}
double get_oc_tjmax(zes_freq_handle_t freqHandle) {
  double tjmax = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcGetTjMax(freqHandle, &tjmax));
  return tjmax;
}
void set_oc_tjmax(zes_freq_handle_t freqHandle, double tjmax) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFrequencyOcSetTjMax(freqHandle, tjmax));
}

} // namespace level_zero_tests
