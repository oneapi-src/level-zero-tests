/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_SYSMAN_RAS_HPP
#define TEST_HARNESS_SYSMAN_RAS_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"
namespace level_zero_tests {

std::vector<zet_sysman_ras_handle_t> get_ras_handles(ze_device_handle_t device,
                                                     uint32_t &count);
zet_ras_properties_t get_ras_properties(zet_sysman_ras_handle_t rasHandle);
zet_ras_config_t get_ras_config(zet_sysman_ras_handle_t rasHandle);
void set_ras_config(zet_sysman_ras_handle_t rasHandle,
                    zet_ras_config_t rasConfig);
zet_ras_details_t get_ras_state(zet_sysman_ras_handle_t rasHandle,
                                ze_bool_t clear);
uint64_t sum_of_ras_errors(zet_ras_details_t rasDetails);
} // namespace level_zero_tests

#endif /* TEST_HARNESS_SYSMAN_RAS_HPP */
