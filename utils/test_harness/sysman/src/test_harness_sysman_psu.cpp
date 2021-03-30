/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_psu_handles_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumPsus(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_psu_handle_t> get_psu_handles(zes_device_handle_t device,
                                              uint32_t &count) {
  if (count == 0) {
    count = get_psu_handles_count(device);
  }
  std::vector<zes_psu_handle_t> psu_handles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumPsus(device, &count, psu_handles.data()));
  return psu_handles;
}

zes_psu_properties_t get_psu_properties(zes_psu_handle_t psuHandle) {
  zes_psu_properties_t properties = {ZES_STRUCTURE_TYPE_PSU_PROPERTIES,
                                     nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPsuGetProperties(psuHandle, &properties));
  return properties;
}
zes_psu_state_t get_psu_state(zes_psu_handle_t psuHandle) {
  zes_psu_state_t state = {ZES_STRUCTURE_TYPE_PSU_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesPsuGetState(psuHandle, &state));
  return state;
}
} // namespace level_zero_tests
