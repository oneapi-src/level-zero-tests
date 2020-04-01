/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DIAGNOSTICS_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DIAGNOSTICS_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_diag_handle_count(ze_device_handle_t device);

std::vector<zet_sysman_diag_handle_t>
get_diag_handles(ze_device_handle_t device, uint32_t &count);

zet_diag_properties_t get_diag_properties(zet_sysman_diag_handle_t diagHandle);

std::vector<zet_diag_test_t> get_diag_tests(zet_sysman_diag_handle_t diagHandle,
                                            uint32_t &count);

zet_diag_result_t run_diag_tests(zet_sysman_diag_handle_t diagHandle,
                                 uint32_t start, uint32_t end);

} // namespace level_zero_tests

#endif
