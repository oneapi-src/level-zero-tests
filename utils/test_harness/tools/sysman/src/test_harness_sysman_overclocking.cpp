/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_oc_capabilities_t get_oc_capabilities(zet_sysman_freq_handle_t freqHandle) {
  zet_oc_capabilities_t capabilities;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcGetCapabilities(freqHandle, &capabilities));
  return capabilities;
}
zet_oc_config_t get_oc_config(zet_sysman_freq_handle_t freqHandle) {
  zet_oc_config_t config;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcGetConfig(freqHandle, &config));
  return config;
}
void set_oc_config(zet_sysman_freq_handle_t freqHandle, zet_oc_config_t &config,
                   ze_bool_t *restart) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcSetConfig(freqHandle, &config, restart));
}
double get_oc_iccmax(zet_sysman_freq_handle_t freqHandle) {
  double icmax;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcGetIccMax(freqHandle, &icmax));
  return icmax;
}
void set_oc_iccmax(zet_sysman_freq_handle_t freqHandle, double icmax) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcSetIccMax(freqHandle, icmax));
}
double get_oc_tjmax(zet_sysman_freq_handle_t freqHandle) {
  double tjmax;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFrequencyOcGetTjMax(freqHandle, &tjmax));
  return tjmax;
}
void set_oc_tjmax(zet_sysman_freq_handle_t freqHandle, double tjmax) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFrequencyOcSetTjMax(freqHandle, tjmax));
}

} // namespace level_zero_tests
