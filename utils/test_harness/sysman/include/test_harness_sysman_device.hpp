/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DEVICE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DEVICE_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

void sysman_device_reset(zes_device_handle_t device);

zes_device_properties_t
get_sysman_device_properties(zes_device_handle_t device);

uint32_t get_processes_count(zes_device_handle_t device);

zes_device_state_t get_device_state(zes_device_handle_t device);

std::vector<zes_process_state_t> get_processes_state(zes_device_handle_t device,
                                                     uint32_t &count);

void sysman_device_reset_ext(zes_device_handle_t device,
                             zes_reset_properties_t &properties);

} // namespace level_zero_tests

#endif
