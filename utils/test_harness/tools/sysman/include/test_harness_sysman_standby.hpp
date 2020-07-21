/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_STANDBY_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_STANDBY_HPP
#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {
uint32_t get_standby_handle_count(zes_device_handle_t device);
std::vector<zes_standby_handle_t>
get_standby_handles(zes_device_handle_t device, uint32_t &count);
zes_standby_promo_mode_t get_standby_mode(zes_standby_handle_t pHandle);
void set_standby_mode(zes_standby_handle_t pHandle,
                      zes_standby_promo_mode_t pMode);
zes_standby_properties_t
get_standby_properties(zes_standby_handle_t pStandbyHandle);

} // namespace level_zero_tests

#endif
