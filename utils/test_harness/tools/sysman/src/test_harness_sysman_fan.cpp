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
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFanGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_fan_handle_t> get_fan_handles(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0) {
    count = get_fan_handle_count(device);
  }
  std::vector<zet_sysman_fan_handle_t> fanHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFanGet(hSysman, &count, fanHandles.data()));
  EXPECT_EQ(fanHandles.size(), count);
  return fanHandles;
}

zet_fan_properties_t get_fan_properties(zet_sysman_fan_handle_t fanHandle) {
  zet_fan_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFanGetProperties(fanHandle, &properties));
  return properties;
}
void get_fan_state(zet_sysman_fan_handle_t fanHandle,
                   zet_fan_speed_units_t units, uint32_t &speed) {
  zet_psu_state_t state;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFanGetState(fanHandle, units, &speed));
}
zet_fan_config_t get_fan_configuration(zet_sysman_fan_handle_t fanHandle) {
  zet_fan_config_t config;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFanGetConfig(fanHandle, &config));
  return config;
}
void set_fan_configuration(zet_sysman_fan_handle_t fanHandle,
                           zet_fan_config_t &config) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFanSetConfig(fanHandle, &config));
}
} // namespace level_zero_tests
