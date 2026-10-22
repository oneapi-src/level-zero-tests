/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_mem_handle_count(zes_device_handle_t device);

std::vector<zes_mem_handle_t> get_mem_handles(zes_device_handle_t device,
                                              uint32_t &count);

zes_mem_properties_t get_mem_properties(zes_mem_handle_t memHandle);

zes_mem_bandwidth_t get_mem_bandwidth(zes_mem_handle_t memHandle);

zes_mem_state_t get_mem_state(zes_mem_handle_t memHandle);

} // namespace level_zero_tests
