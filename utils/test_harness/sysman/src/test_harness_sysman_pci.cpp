/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zes_pci_state_t get_pci_state(zes_device_handle_t device) {
  zes_pci_state_t pciState = {ZES_STRUCTURE_TYPE_PCI_STATE, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDevicePciGetState(device, &pciState));
  return pciState;
}

zes_pci_properties_t get_pci_properties(zes_device_handle_t device) {
  zes_pci_properties_t pciProps = {ZES_STRUCTURE_TYPE_PCI_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDevicePciGetProperties(device, &pciProps));
  return pciProps;
}
uint32_t get_pci_bar_count(zes_device_handle_t device) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDevicePciGetBars(device, &pCount, nullptr));
  EXPECT_GT(pCount, 0);
  return pCount;
}

std::vector<zes_pci_bar_properties_t> get_pci_bars(zes_device_handle_t device,
                                                   uint32_t *pCount) {
  if (*pCount == 0)
    *pCount = get_pci_bar_count(device);
  std::vector<zes_pci_bar_properties_t> pciBarProps(*pCount);
  for (uint32_t i = 0; i < *pCount; i++) {
    pciBarProps[i].stype = ZES_STRUCTURE_TYPE_PCI_BAR_PROPERTIES;
    pciBarProps[i].pNext = nullptr;
  }
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDevicePciGetBars(device, pCount, pciBarProps.data()));
  return pciBarProps;
}
zes_pci_stats_t get_pci_stats(zes_device_handle_t device) {
  zes_pci_stats_t pciStats = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDevicePciGetStats(device, &pciStats));
  return pciStats;
}
} // namespace level_zero_tests
