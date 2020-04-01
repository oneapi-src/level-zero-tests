/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_MEMORY_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_MEMORY_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_mem_handle_count(ze_device_handle_t device);

std::vector<zet_sysman_mem_handle_t> get_mem_handles(ze_device_handle_t device,
                                                     uint32_t &count);

zet_mem_properties_t get_mem_properties(zet_sysman_mem_handle_t memHandle);

zet_mem_bandwidth_t get_mem_bandwidth(zet_sysman_mem_handle_t memHandle);

zet_mem_state_t get_mem_state(zet_sysman_mem_handle_t memHandle);

} // namespace level_zero_tests

#endif
