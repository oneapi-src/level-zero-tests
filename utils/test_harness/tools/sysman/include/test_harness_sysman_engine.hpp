/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ENGINE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ENGINE_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_engine_handle_count(ze_device_handle_t hDevice);

std::vector<zet_sysman_engine_handle_t>
get_engine_handles(ze_device_handle_t hDevice, uint32_t &count);

zet_engine_properties_t
get_engine_properties(zet_sysman_engine_handle_t hEngine);

zet_engine_stats_t get_engine_activity(zet_sysman_engine_handle_t hEngine);

} // namespace level_zero_tests

#endif
