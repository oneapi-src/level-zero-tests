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

uint32_t get_fan_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumFans(device, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zes_fan_handle_t> get_fan_handles(ze_device_handle_t device,
                                              uint32_t &count) {
  if (count == 0) {
    count = get_fan_handle_count(device);
  }
  std::vector<zes_fan_handle_t> fan_handles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFans(device, &count, fan_handles.data()));
  return fan_handles;
}

zes_fan_properties_t get_fan_properties(zes_fan_handle_t fan_handle) {
  zes_fan_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFanGetProperties(fan_handle, &properties));
  return properties;
}
void get_fan_state(zes_fan_handle_t fan_handle, zes_fan_speed_units_t units,
                   int32_t &speed) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFanGetState(fan_handle, units, &speed));
}
zes_fan_config_t get_fan_configuration(zes_fan_handle_t fan_handle) {
  zes_fan_config_t config;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFanGetConfig(fan_handle, &config));
  return config;
}
void set_fan_default_mode(zes_fan_handle_t fan_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFanSetDefaultMode(fan_handle));
}
void set_fan_fixed_speed_mode(zes_fan_handle_t fan_handle,
                              zes_fan_speed_t &speed) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFanSetFixedSpeedMode(fan_handle, &speed));
}
void set_fan_speed_table_mode(zes_fan_handle_t fan_handle,
                              zes_fan_speed_table_t &speedTable) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFanSetSpeedTableMode(fan_handle, &speedTable));
}
} // namespace level_zero_tests
