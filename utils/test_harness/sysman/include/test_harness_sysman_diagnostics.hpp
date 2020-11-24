/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DIAGNOSTICS_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DIAGNOSTICS_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_diag_handle_count(zes_device_handle_t device);

std::vector<zes_diag_handle_t> get_diag_handles(ze_device_handle_t device,
                                                uint32_t &count);

zes_diag_properties_t get_diag_properties(zes_diag_handle_t diagHandle);

std::vector<zes_diag_test_t> get_diag_tests(zes_diag_handle_t diagHandle,
                                            uint32_t &count);

zes_diag_result_t run_diag_tests(zes_diag_handle_t diagHandle, uint32_t start,
                                 uint32_t end);

} // namespace level_zero_tests

#endif
