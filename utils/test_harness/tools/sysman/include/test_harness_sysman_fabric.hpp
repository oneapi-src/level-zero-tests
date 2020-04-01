/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FABRIC_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FABRIC_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

uint32_t get_fabric_port_handles_count(zet_sysman_handle_t hSysman);

std::vector<zet_sysman_fabric_port_handle_t>
get_fabric_port_handles(ze_device_handle_t device, uint32_t &count);

zet_fabric_port_properties_t
get_fabric_port_properties(zet_sysman_fabric_port_handle_t fabricPortHandle);

void set_fabric_port_config(zet_sysman_fabric_port_handle_t fabricPortHandle,
                            zet_fabric_port_config_t config);

zet_fabric_port_config_t
get_fabric_port_config(zet_sysman_fabric_port_handle_t fabricPortHandle);

zet_fabric_port_state_t
get_fabric_port_state(zet_sysman_fabric_port_handle_t fabricPortHandle);

zet_fabric_port_throughput_t
get_fabric_port_throughput(zet_sysman_fabric_port_handle_t fabricPortHandle);

} // namespace level_zero_tests

#endif
