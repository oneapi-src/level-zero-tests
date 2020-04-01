/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DEVICE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_DEVICE_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

void sysman_device_reset(ze_device_handle_t device);

zet_sysman_properties_t get_sysman_device_properties(ze_device_handle_t device);

} // namespace level_zero_tests

#endif
