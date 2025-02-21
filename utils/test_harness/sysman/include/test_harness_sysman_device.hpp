/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
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

zes_uuid_t get_sysman_device_uuid(zes_device_handle_t device);

uint32_t get_zes_device_count(zes_driver_handle_t driver);
std::vector<zes_device_handle_t> get_zes_devices(uint32_t count,
                                                 zes_driver_handle_t driver);
std::vector<zes_subdevice_exp_properties_t>
get_sysman_subdevice_properties(zes_device_handle_t device, uint32_t &count);

zes_device_handle_t get_sysman_device_by_uuid(zes_driver_handle_t driver,
                                              zes_uuid_t uuid,
                                              ze_bool_t &on_sub_device,
                                              uint32_t &sub_device_id);

uint32_t get_processes_count(zes_device_handle_t device);

zes_device_state_t get_device_state(zes_device_handle_t device);

std::vector<zes_process_state_t> get_processes_state(zes_device_handle_t device,
                                                     uint32_t &count);

void sysman_device_reset_ext(zes_device_handle_t device, ze_bool_t force,
                             zes_reset_type_t type);

bool is_uuid_pair_equal(uint8_t *uuid1, uint8_t *uuid2);

ze_device_handle_t get_core_device_by_uuid(uint8_t *uuid);

} // namespace level_zero_tests

#endif
