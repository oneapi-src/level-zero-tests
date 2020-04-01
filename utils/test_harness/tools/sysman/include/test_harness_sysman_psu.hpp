/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PSU_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PSU_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_psu_handles_count(ze_device_handle_t device);

std::vector<zet_sysman_psu_handle_t> get_psu_handles(ze_device_handle_t device,
                                                     uint32_t &count);

zet_psu_properties_t get_psu_properties(zet_sysman_psu_handle_t psuHandle);

zet_psu_state_t get_psu_state(zet_sysman_psu_handle_t psuHandle);
} // namespace level_zero_tests

#endif
