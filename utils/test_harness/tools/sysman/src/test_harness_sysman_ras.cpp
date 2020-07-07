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

uint32_t get_ras_handles_count(zet_sysman_handle_t hSysman) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanRasGet(hSysman, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zet_sysman_ras_handle_t> get_ras_handles(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0) {
    count = get_ras_handles_count(hSysman);
  }
  std::vector<zet_sysman_ras_handle_t> rasHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanRasGet(hSysman, &count, rasHandles.data()));
  return rasHandles;
}

zet_ras_properties_t get_ras_properties(zet_sysman_ras_handle_t rasHandle) {
  zet_ras_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanRasGetProperties(rasHandle, &properties));
  return properties;
}

zet_ras_config_t get_ras_config(zet_sysman_ras_handle_t rasHandle) {
  zet_ras_config_t rasConfig;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanRasGetConfig(rasHandle, &rasConfig));
  return rasConfig;
}

void set_ras_config(zet_sysman_ras_handle_t rasHandle,
                    zet_ras_config_t rasConfig) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanRasSetConfig(rasHandle, &rasConfig));
}

uint64_t sum_of_ras_errors(zet_ras_details_t rasDetails) {
  return (rasDetails.numCacheErrors + rasDetails.numComputeErrors +
          rasDetails.numDisplayErrors + rasDetails.numDriverErrors +
          rasDetails.numNonComputeErrors + rasDetails.numProgrammingErrors +
          rasDetails.numResets);
}
zet_ras_details_t get_ras_state(zet_sysman_ras_handle_t rasHandle,
                                ze_bool_t clear) {
  uint64_t tErrors = 0;
  zet_ras_details_t rasDetails;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanRasGetState(rasHandle, clear, &tErrors, &rasDetails));
  EXPECT_EQ(tErrors, sum_of_ras_errors(rasDetails));
  return rasDetails;
}
} // namespace level_zero_tests
