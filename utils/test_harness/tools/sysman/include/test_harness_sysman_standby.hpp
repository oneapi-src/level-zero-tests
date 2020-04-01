/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_STANDBY_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_STANDBY_HPP
#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {
uint32_t get_standby_handle_count(ze_device_handle_t device);
std::vector<zet_sysman_standby_handle_t>
get_standby_handles(ze_device_handle_t device, uint32_t &count);
zet_standby_promo_mode_t get_standby_mode(zet_sysman_standby_handle_t pHnadle);
void set_standby_mode(zet_sysman_standby_handle_t pHnadle,
                      zet_standby_promo_mode_t pMode);
zet_standby_properties_t
get_standby_properties(zet_sysman_standby_handle_t pStandbyHandle);

} // namespace level_zero_tests

#endif
