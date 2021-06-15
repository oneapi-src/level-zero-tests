/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_SCHEDULER_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_SCHEDULER_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_scheduler_handle_count(zes_device_handle_t device);
std::vector<zes_sched_handle_t>
get_scheduler_handles(zes_device_handle_t device, uint32_t &count);

zes_sched_mode_t get_scheduler_current_mode(zes_sched_handle_t hScheduler);

zes_sched_timeout_properties_t
get_timeout_properties(zes_sched_handle_t hScheduler, ze_bool_t pDefault);

zes_sched_timeslice_properties_t
get_timeslice_properties(zes_sched_handle_t hScheduler, ze_bool_t pDefault);

void set_timeout_mode(zes_sched_handle_t hScheduler,
                      zes_sched_timeout_properties_t &properties);

void set_timeslice_mode(zes_sched_handle_t hScheduler,
                        zes_sched_timeslice_properties_t &properties);

void set_exclusive_mode(zes_sched_handle_t hScheduler);

ze_bool_t set_compute_unit_debug_mode(zes_sched_handle_t hScheduler);

zes_sched_properties_t get_scheduler_properties(zes_sched_handle_t hScheduler);

} // namespace level_zero_tests

#endif
