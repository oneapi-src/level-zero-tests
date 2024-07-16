/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_SYSMAN_RAS_HPP
#define TEST_HARNESS_SYSMAN_RAS_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"
namespace level_zero_tests {

std::vector<zes_ras_handle_t> get_ras_handles(zes_device_handle_t device,
                                              uint32_t &count);
zes_ras_properties_t get_ras_properties(zes_ras_handle_t rasHandle);
zes_ras_config_t get_ras_config(zes_ras_handle_t rasHandle);
void set_ras_config(zes_ras_handle_t rasHandle, zes_ras_config_t rasConfig);
zes_ras_state_t get_ras_state(zes_ras_handle_t rasHandle, ze_bool_t clear);
uint64_t sum_of_ras_errors(zes_ras_state_t state);
std::vector<zes_ras_state_exp_t> ras_get_state_exp(zes_ras_handle_t ras_handle);
void ras_clear_state_exp(zes_ras_handle_t ras_handle,
                         zes_ras_error_category_exp_t category);
} // namespace level_zero_tests

#endif
