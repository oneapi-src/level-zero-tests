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

zet_sched_mode_t get_scheduler_current_mode(ze_device_handle_t hDevice) {
  zet_sched_mode_t cur_mode;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanSchedulerGetCurrentMode(hSysman, &cur_mode));
  return cur_mode;
}

zet_sched_timeout_properties_t
get_timeout_properties(ze_device_handle_t hDevice, ze_bool_t pDefault) {
  zet_sched_timeout_properties_t properties;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanSchedulerGetTimeoutModeProperties(
                                   hSysman, pDefault, &properties));
  return properties;
}

zet_sched_timeslice_properties_t
get_timeslice_properties(ze_device_handle_t hDevice, ze_bool_t pDefault) {
  zet_sched_timeslice_properties_t properties;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanSchedulerGetTimesliceModeProperties(
                                   hSysman, pDefault, &properties));
  return properties;
}

void set_timeout_mode(ze_device_handle_t hDevice,
                      zet_sched_timeout_properties_t &properties) {
  ze_bool_t reboot;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanSchedulerSetTimeoutMode(hSysman, &properties, &reboot));
}

void set_timeslice_mode(ze_device_handle_t hDevice,
                        zet_sched_timeslice_properties_t &properties) {
  ze_bool_t reboot;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanSchedulerSetTimesliceMode(hSysman, &properties, &reboot));
}

void set_exclusive_mode(ze_device_handle_t hDevice) {
  ze_bool_t reboot;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanSchedulerSetExclusiveMode(hSysman, &reboot));
}

void set_compute_unit_debug_mode(ze_device_handle_t hDevice) {
  ze_bool_t reboot;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(hDevice);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanSchedulerSetExclusiveMode(hSysman, &reboot));
}
} // namespace level_zero_tests
