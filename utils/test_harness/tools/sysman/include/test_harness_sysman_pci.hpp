/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PCI_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PCI_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

#define MAX_DOMAINs 65536
#define MAX_BUSES_PER_DOMAIN 256
#define MAX_DEVICES_PER_BUS 32
#define MAX_FUNCTIONS_PER_DEVICE 8

#define PCI_SPEED_MAX_LINK_GEN 5
#define PCI_SPEED_MAX_LANE_WIDTH 255

namespace level_zero_tests {
zet_pci_state_t get_pci_state(ze_device_handle_t device);
zet_pci_properties_t get_pci_properties(ze_device_handle_t device);
uint32_t get_pci_bar_count(ze_device_handle_t device);
std::vector<zet_pci_bar_properties_t> get_pci_bars(ze_device_handle_t device,
                                                   uint32_t *pCount);
zet_pci_stats_t get_pci_stats(ze_device_handle_t device);
} // namespace level_zero_tests
#endif
