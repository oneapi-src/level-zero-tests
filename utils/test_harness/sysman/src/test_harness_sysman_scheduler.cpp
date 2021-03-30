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

uint32_t get_scheduler_handle_count(zes_device_handle_t device) {
  uint32_t p_count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumSchedulers(device, &p_count, nullptr));
  EXPECT_GE(p_count, 0);
  return p_count;
}

std::vector<zes_sched_handle_t>
get_scheduler_handles(zes_device_handle_t device, uint32_t &p_count) {
  if (p_count == 0)
    p_count = get_scheduler_handle_count(device);
  std::vector<zes_sched_handle_t> pSchedHandles(p_count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumSchedulers(device, &p_count, pSchedHandles.data()));
  return pSchedHandles;
}

zes_sched_mode_t get_scheduler_current_mode(zes_sched_handle_t hScheduler) {
  zes_sched_mode_t cur_mode = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerGetCurrentMode(hScheduler, &cur_mode));
  return cur_mode;
}

zes_sched_timeout_properties_t
get_timeout_properties(zes_sched_handle_t hScheduler, ze_bool_t pDefault) {
  zes_sched_timeout_properties_t properties = {
      ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesSchedulerGetTimeoutModeProperties(
                                   hScheduler, pDefault, &properties));
  return properties;
}

zes_sched_timeslice_properties_t
get_timeslice_properties(zes_sched_handle_t hScheduler, ze_bool_t pDefault) {
  zes_sched_timeslice_properties_t properties = {
      ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesSchedulerGetTimesliceModeProperties(
                                   hScheduler, pDefault, &properties));
  return properties;
}

void set_timeout_mode(zes_sched_handle_t hScheduler,
                      zes_sched_timeout_properties_t &properties) {
  ze_bool_t reload = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerSetTimeoutMode(hScheduler, &properties, &reload));
}

void set_timeslice_mode(zes_sched_handle_t hScheduler,
                        zes_sched_timeslice_properties_t &properties) {
  ze_bool_t reload = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerSetTimesliceMode(hScheduler, &properties, &reload));
}

void set_exclusive_mode(zes_sched_handle_t hScheduler) {
  ze_bool_t reload = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerSetExclusiveMode(hScheduler, &reload));
}

void set_compute_unit_debug_mode(zes_sched_handle_t hScheduler) {
  ze_bool_t reload = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerSetComputeUnitDebugMode(hScheduler, &reload));
}

zes_sched_properties_t get_scheduler_properties(zes_sched_handle_t hScheduler) {
  zes_sched_properties_t properties = {ZES_STRUCTURE_TYPE_SCHED_PROPERTIES,
                                       nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesSchedulerGetProperties(hScheduler, &properties));
  return properties;
}
} // namespace level_zero_tests
