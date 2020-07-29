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

void sysman_device_reset(zes_device_handle_t device) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceReset(device, true));
}

zes_device_properties_t
get_sysman_device_properties(zes_device_handle_t device) {
  zes_device_properties_t properties = {ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                        nullptr};

  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceGetProperties(device, &properties));

  return properties;
}

uint32_t get_processes_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceProcessesGetState(device, &count, nullptr));
  return count;
}

std::vector<zes_process_state_t> get_processes_state(zes_device_handle_t device,
                                                     uint32_t &count) {
  if (count == 0)
    count = get_processes_count(device);
  std::vector<zes_process_state_t> processes(count);
  for (uint32_t i = 0; i < count; i++) {
    processes[i].stype = ZES_STRUCTURE_TYPE_PROCESS_STATE;
    processes[i].pNext = nullptr;
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceProcessesGetState(device, &count, processes.data()));
  return processes;
}

zes_device_state_t get_device_state(zes_device_handle_t device) {
  zes_device_state_t state = {ZES_STRUCTURE_TYPE_DEVICE_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceGetState(device, &state));
  return state;
}

} // namespace level_zero_tests
