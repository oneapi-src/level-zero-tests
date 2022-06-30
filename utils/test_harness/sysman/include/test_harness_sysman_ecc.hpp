/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ECC_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_ECC_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

ze_bool_t get_ecc_available(zes_device_handle_t hDevice);

ze_bool_t get_ecc_configurable(zes_device_handle_t hDevice);

zes_device_ecc_properties_t get_ecc_state(zes_device_handle_t hDevice);

zes_device_ecc_properties_t set_ecc_state(zes_device_handle_t hDevice,
                                          zes_device_ecc_desc_t &newState);

} // namespace level_zero_tests

#endif
