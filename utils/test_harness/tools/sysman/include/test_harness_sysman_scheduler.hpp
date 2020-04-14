/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_SCHEDULER_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_SCHEDULER_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

zet_sched_mode_t get_scheduler_current_mode(ze_device_handle_t hDevice);

zet_sched_timeout_properties_t
get_timeout_properties(ze_device_handle_t hDevice, ze_bool_t pDefault);

zet_sched_timeslice_properties_t
get_timeslice_properties(ze_device_handle_t hDevice, ze_bool_t pDefault);

void set_timeout_mode(ze_device_handle_t hDevice,
                      zet_sched_timeout_properties_t &properties);

void set_timeslice_mode(ze_device_handle_t hDevice,
                        zet_sched_timeslice_properties_t &properties);

void set_exclusive_mode(ze_device_handle_t hDevice);

void set_compute_unit_debug_mode(ze_device_handle_t hDevice);

} // namespace level_zero_tests

#endif
