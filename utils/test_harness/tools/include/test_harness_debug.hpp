/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_DEBUG_HPP
#define TEST_HARNESS_DEBUG_HPP

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

zet_device_debug_properties_t get_debug_properties(ze_device_handle_t device);

zet_debug_session_handle_t debug_attach(const ze_device_handle_t &device,
                                        const zet_debug_config_t &debug_config);

void debug_detach(const zet_debug_session_handle_t &debug_session);

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_DEBUG_HPP */
