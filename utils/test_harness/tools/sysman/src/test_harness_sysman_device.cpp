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

void sysman_device_reset(ze_device_handle_t device) {

  ze_result_t status;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanDeviceReset(hSysman));
}

zet_sysman_properties_t
get_sysman_device_properties(ze_device_handle_t device) {
  zet_sysman_properties_t properties;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDeviceGetProperties(hSysman, &properties));

  return properties;
}

uint32_t get_processes_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanProcessesGetState(hSysman, &count, nullptr));
  return count;
}

std::vector<zet_process_state_t> get_processes_state(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0)
    count = get_processes_count(device);
  std::vector<zet_process_state_t> processes(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanProcessesGetState(hSysman, &count, processes.data()));
  return processes;
}

} // namespace level_zero_tests
