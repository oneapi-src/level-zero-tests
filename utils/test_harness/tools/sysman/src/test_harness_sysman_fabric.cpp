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

uint32_t get_fabric_port_handles_count(zet_sysman_handle_t hSysman) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGet(hSysman, &count, nullptr));
  return count;
}

std::vector<zet_sysman_fabric_port_handle_t>
get_fabric_port_handles(ze_device_handle_t device, uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0) {
    count = get_fabric_port_handles_count(hSysman);
  }
  std::vector<zet_sysman_fabric_port_handle_t> fabricPortHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGet(hSysman, &count, fabricPortHandles.data()));
  EXPECT_EQ(fabricPortHandles.size(), count);
  return fabricPortHandles;
}

zet_fabric_port_properties_t
get_fabric_port_properties(zet_sysman_fabric_port_handle_t fabricPortHandle) {
  zet_fabric_port_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGetProperties(fabricPortHandle, &properties));
  return properties;
}

zet_fabric_port_config_t
get_fabric_port_config(zet_sysman_fabric_port_handle_t fabricPortHandle) {
  zet_fabric_port_config_t config;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGetConfig(fabricPortHandle, &config));
  return config;
}

void set_fabric_port_config(zet_sysman_fabric_port_handle_t fabricPortHandle,
                            zet_fabric_port_config_t config) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortSetConfig(fabricPortHandle, &config));
}

zet_fabric_port_state_t
get_fabric_port_state(zet_sysman_fabric_port_handle_t fabricPortHandle) {
  zet_fabric_port_state_t state;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGetState(fabricPortHandle, &state));
  return state;
}

zet_fabric_port_throughput_t
get_fabric_port_throughput(zet_sysman_fabric_port_handle_t fabricPortHandle) {
  zet_fabric_port_throughput_t throughput;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanFabricPortGetThroughput(fabricPortHandle, &throughput));
  return throughput;
}

} // namespace level_zero_tests
