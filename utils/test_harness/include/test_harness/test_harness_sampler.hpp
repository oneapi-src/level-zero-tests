/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>

namespace level_zero_tests {

ze_sampler_handle_t create_sampler();

ze_sampler_handle_t create_sampler(ze_sampler_address_mode_t addrmode,
                                   ze_sampler_filter_mode_t filtmode,
                                   ze_bool_t normalized);

void destroy_sampler(ze_sampler_handle_t sampler);

}; // namespace level_zero_tests
