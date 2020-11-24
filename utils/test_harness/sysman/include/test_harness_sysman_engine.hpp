/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ENGINE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ENGINE_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_engine_handle_count(zes_device_handle_t hDevice);

std::vector<zes_engine_handle_t> get_engine_handles(zes_device_handle_t hDevice,
                                                    uint32_t &count);

zes_engine_properties_t get_engine_properties(zes_engine_handle_t hEngine);

zes_engine_stats_t get_engine_activity(zes_engine_handle_t hEngine);

} // namespace level_zero_tests

#endif
