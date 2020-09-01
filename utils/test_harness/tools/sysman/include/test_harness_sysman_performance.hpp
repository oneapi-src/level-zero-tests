/* Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PERFORMANCE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PERFORMANCE_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_performance_handle_count(zes_device_handle_t device);

std::vector<zes_perf_handle_t> get_performance_handles(zes_device_handle_t device,
                                                uint32_t &count);

zes_perf_properties_t get_performance_properties(zes_perf_handle_t performanceHandle);

double get_performance_config(zes_perf_handle_t performanceHandle);

void set_performance_config(zes_perf_handle_t performanceHandle,double factor);

} // namespace level_zero_tests

#endif
