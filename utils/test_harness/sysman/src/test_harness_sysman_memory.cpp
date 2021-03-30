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

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_mem_handle_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumMemoryModules(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_mem_handle_t> get_mem_handles(zes_device_handle_t device,
                                              uint32_t &count) {
  if (count == 0)
    count = get_mem_handle_count(device);
  std::vector<zes_mem_handle_t> mem_handles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumMemoryModules(device, &count, mem_handles.data()));
  return mem_handles;
}

zes_mem_properties_t get_mem_properties(zes_mem_handle_t memHandle) {
  zes_mem_properties_t properties = {ZES_STRUCTURE_TYPE_MEM_PROPERTIES,
                                     nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesMemoryGetProperties(memHandle, &properties));
  return properties;
}

zes_mem_bandwidth_t get_mem_bandwidth(zes_mem_handle_t memHandle) {
  zes_mem_bandwidth_t bandwidth = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesMemoryGetBandwidth(memHandle, &bandwidth));
  return bandwidth;
}

zes_mem_state_t get_mem_state(zes_mem_handle_t memHandle) {
  zes_mem_state_t state = {ZES_STRUCTURE_TYPE_MEM_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesMemoryGetState(memHandle, &state));
  return state;
}

} // namespace level_zero_tests
