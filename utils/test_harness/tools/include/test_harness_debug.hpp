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

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_DEBUG_HPP */
