/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_VF_MANAGEMENT_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_VF_MANAGEMENT_HPP

#include <level_zero/zes_api.h>

namespace level_zero_tests {

uint32_t get_vf_handles_count(zes_device_handle_t device);
std::vector<zes_vf_handle_t> get_vf_handles(zes_device_handle_t device,
                                            uint32_t &count);
zes_vf_exp_capabilities_t get_vf_capabilities(zes_vf_handle_t vf_handle);
uint32_t get_vf_mem_util_count(zes_vf_handle_t vf_handle);
std::vector<zes_vf_util_mem_exp2_t> get_vf_mem_util(zes_vf_handle_t vf_handle,
                                                    uint32_t &count);
uint32_t get_vf_engine_util_count(zes_vf_handle_t vf_handle);
std::vector<zes_vf_util_engine_exp2_t>
get_vf_engine_util(zes_vf_handle_t vf_handle, uint32_t &count);

} // namespace level_zero_tests

#endif