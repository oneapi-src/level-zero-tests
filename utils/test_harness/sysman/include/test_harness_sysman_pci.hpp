/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PCI_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_PCI_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

#define MAX_DOMAINs 65536
#define MAX_BUSES_PER_DOMAIN 256
#define MAX_DEVICES_PER_BUS 32
#define MAX_FUNCTIONS_PER_DEVICE 8

#define PCI_SPEED_MAX_LINK_GEN 5
#define PCI_SPEED_MAX_LANE_WIDTH 255

namespace level_zero_tests {
zes_pci_state_t get_pci_state(zes_device_handle_t device);
zes_pci_properties_t get_pci_properties(zes_device_handle_t device);
uint32_t get_pci_bar_count(zes_device_handle_t device);
std::vector<zes_pci_bar_properties_t> get_pci_bars(zes_device_handle_t device,
                                                   uint32_t *pCount);
std::vector<zes_pci_bar_properties_t>
get_pci_bars_extension(zes_device_handle_t device, uint32_t *pCount);
zes_pci_stats_t get_pci_stats(zes_device_handle_t device);
} // namespace level_zero_tests
#endif
