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

uint32_t get_ras_handles_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumRasErrorSets(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_ras_handle_t> get_ras_handles(zes_device_handle_t device,
                                              uint32_t &count) {
  if (count == 0) {
    count = get_ras_handles_count(device);
  }
  std::vector<zes_ras_handle_t> ras_handles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumRasErrorSets(device, &count, ras_handles.data()));
  return ras_handles;
}

zes_ras_properties_t get_ras_properties(zes_ras_handle_t rasHandle) {
  zes_ras_properties_t properties = {ZES_STRUCTURE_TYPE_RAS_PROPERTIES,
                                     nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesRasGetProperties(rasHandle, &properties));
  return properties;
}

zes_ras_config_t get_ras_config(zes_ras_handle_t rasHandle) {
  zes_ras_config_t rasConfig = {ZES_STRUCTURE_TYPE_RAS_CONFIG, nullptr};
  zes_ras_state_t state = {ZES_STRUCTURE_TYPE_RAS_STATE, nullptr};
  rasConfig.detailedThresholds = state;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesRasGetConfig(rasHandle, &rasConfig));
  return rasConfig;
}

void set_ras_config(zes_ras_handle_t rasHandle, zes_ras_config_t rasConfig) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesRasSetConfig(rasHandle, &rasConfig));
}

uint64_t sum_of_ras_errors(zes_ras_state_t rasState) {
  uint64_t sum = 0;
  for (uint32_t i = 0; i < ZES_MAX_RAS_ERROR_CATEGORY_COUNT; i++) {
    sum += rasState.category[i];
  }
  return sum;
}
zes_ras_state_t get_ras_state(zes_ras_handle_t rasHandle, ze_bool_t clear) {
  zes_ras_state_t state = {ZES_STRUCTURE_TYPE_RAS_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesRasGetState(rasHandle, clear, &state));
  return state;
}
} // namespace level_zero_tests
