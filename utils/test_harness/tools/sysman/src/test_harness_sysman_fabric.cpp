/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_fabric_port_handles_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFabricPorts(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_fabric_port_handle_t>
get_fabric_port_handles(zes_device_handle_t device, uint32_t &count) {
  if (count == 0) {
    count = get_fabric_port_handles_count(device);
  }
  std::vector<zes_fabric_port_handle_t> fabricPortHandles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumFabricPorts(device, &count, fabricPortHandles.data()));
  return fabricPortHandles;
}

zes_fabric_port_properties_t
get_fabric_port_properties(zes_fabric_port_handle_t fabricPortHandle) {
  zes_fabric_port_properties_t properties = {
      ZES_STRUCTURE_TYPE_FABRIC_PORT_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFabricPortGetProperties(fabricPortHandle, &properties));
  return properties;
}

zes_fabric_port_config_t
get_fabric_port_config(zes_fabric_port_handle_t fabricPortHandle) {
  zes_fabric_port_config_t config = {ZES_STRUCTURE_TYPE_FABRIC_PORT_CONFIG,
                                     nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFabricPortGetConfig(fabricPortHandle, &config));
  return config;
}

void set_fabric_port_config(zes_fabric_port_handle_t fabricPortHandle,
                            zes_fabric_port_config_t config) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFabricPortSetConfig(fabricPortHandle, &config));
}

zes_fabric_port_state_t
get_fabric_port_state(zes_fabric_port_handle_t fabricPortHandle) {
  zes_fabric_port_state_t state = {ZES_STRUCTURE_TYPE_FABRIC_PORT_STATE,
                                   nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesFabricPortGetState(fabricPortHandle, &state));
  return state;
}

zes_fabric_port_throughput_t
get_fabric_port_throughput(zes_fabric_port_handle_t fabricPortHandle) {
  zes_fabric_port_throughput_t throughput = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFabricPortGetThroughput(fabricPortHandle, &throughput));
  return throughput;
}

zes_fabric_link_type_t
get_fabric_port_link(zes_fabric_port_handle_t fabricPortHandle) {
  zes_fabric_link_type_t link = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesFabricPortGetLinkType(fabricPortHandle, &link));
  return link;
}

} // namespace level_zero_tests
