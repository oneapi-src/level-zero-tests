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

uint32_t get_mem_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanMemoryGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_mem_handle_t> get_mem_handles(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0)
    count = get_mem_handle_count(device);
  std::vector<zet_sysman_mem_handle_t> memHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanMemoryGet(hSysman, &count, memHandles.data()));
  return memHandles;
}

zet_mem_properties_t get_mem_properties(zet_sysman_mem_handle_t memHandle) {
  zet_mem_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanMemoryGetProperties(memHandle, &properties));
  return properties;
}

zet_mem_bandwidth_t get_mem_bandwidth(zet_sysman_mem_handle_t memHandle) {
  zet_mem_bandwidth_t bandwidth;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanMemoryGetBandwidth(memHandle, &bandwidth));
  return bandwidth;
}

zet_mem_state_t get_mem_state(zet_sysman_mem_handle_t memHandle) {
  zet_mem_state_t state;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanMemoryGetState(memHandle, &state));
  return state;
}

} // namespace level_zero_tests
