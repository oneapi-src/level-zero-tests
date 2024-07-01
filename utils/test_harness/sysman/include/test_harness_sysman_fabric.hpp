/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FABRIC_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_FABRIC_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_fabric_port_handles_count(zes_device_handle_t device);

std::vector<zes_fabric_port_handle_t>
get_fabric_port_handles(zes_device_handle_t device, uint32_t &count);

zes_fabric_port_properties_t
get_fabric_port_properties(zes_fabric_port_handle_t fabricPortHandle);

void set_fabric_port_config(zes_fabric_port_handle_t fabricPortHandle,
                            zes_fabric_port_config_t config);

zes_fabric_port_config_t
get_fabric_port_config(zes_fabric_port_handle_t fabricPortHandle);

zes_fabric_port_state_t
get_fabric_port_state(zes_fabric_port_handle_t fabricPortHandle);

zes_fabric_port_throughput_t
get_fabric_port_throughput(zes_fabric_port_handle_t fabricPortHandle);

std::vector<zes_fabric_port_throughput_t>
get_multiport_throughputs(zes_device_handle_t device, uint32_t count,
                          zes_fabric_port_handle_t *fabric_port_handle);

zes_fabric_link_type_t
get_fabric_port_link(zes_fabric_port_handle_t fabricPortHandle);

} // namespace level_zero_tests

#endif
