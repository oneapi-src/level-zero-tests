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

uint32_t get_temp_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumTemperatureSensors(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}
std::vector<zes_temp_handle_t> get_temp_handles(ze_device_handle_t device,
                                                uint32_t &count) {
  if (count == 0) {
    count = get_temp_handle_count(device);
  }
  std::vector<zes_temp_handle_t> temp_handles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumTemperatureSensors(
                                   device, &count, temp_handles.data()));
  return temp_handles;
}

zes_temp_properties_t get_temp_properties(zes_temp_handle_t tempHandle) {
  zes_temp_properties_t properties = {ZES_STRUCTURE_TYPE_TEMP_PROPERTIES,
                                      nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesTemperatureGetProperties(tempHandle, &properties));
  return properties;
}

double get_temp_state(zes_temp_handle_t tempHandle) {
  double temp = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesTemperatureGetState(tempHandle, &temp));
  return temp;
}

zes_temp_config_t get_temp_config(zes_temp_handle_t tempHandle) {
  zes_temp_config_t config = {ZES_STRUCTURE_TYPE_TEMP_CONFIG, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesTemperatureGetConfig(tempHandle, &config));
  return config;
}
ze_result_t set_temp_config(zes_temp_handle_t tempHandle,
                            zes_temp_config_t &config) {
  ze_result_t result = zesTemperatureSetConfig(tempHandle, &config);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  return result;
}
} // namespace level_zero_tests
