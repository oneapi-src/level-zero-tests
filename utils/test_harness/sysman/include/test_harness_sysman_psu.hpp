/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_psu_handles_count(zes_device_handle_t device);

std::vector<zes_psu_handle_t> get_psu_handles(zes_device_handle_t device,
                                              uint32_t &count);

zes_psu_properties_t get_psu_properties(zes_psu_handle_t psuHandle);

zes_psu_state_t get_psu_state(zes_psu_handle_t psuHandle);
} // namespace level_zero_tests
