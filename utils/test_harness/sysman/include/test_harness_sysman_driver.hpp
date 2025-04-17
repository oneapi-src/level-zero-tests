/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DRIVER_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DRIVER_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

ze_result_t get_driver_ext_properties(zes_driver_handle_t driver,
                                      uint32_t *count);

void get_driver_ext_properties(
    zes_driver_handle_t driver, uint32_t *count,
    std::vector<zes_driver_extension_properties_t> &ext_properties);

void get_driver_ext_function_address(zes_driver_handle_t driver,
                                     const char *function_name);
} // namespace level_zero_tests

#endif