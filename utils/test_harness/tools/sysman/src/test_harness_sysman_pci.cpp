/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_pci_state_t get_pci_state(ze_device_handle_t device) {
  zet_pci_state_t PciState;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanPciGetState(hSysmanDevice, &PciState));
  return PciState;
}

zet_pci_properties_t get_pci_properties(ze_device_handle_t device) {
  zet_pci_properties_t PciProps;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPciGetProperties(hSysmanDevice, &PciProps));
  return PciProps;
}
uint32_t get_pci_bar_count(ze_device_handle_t device) {
  uint32_t pCount = 0;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPciGetBars(hSysmanDevice, &pCount, nullptr));
  EXPECT_GT(pCount, 0);
  return pCount;
}

std::vector<zet_pci_bar_properties_t> get_pci_bars(ze_device_handle_t device,
                                                   uint32_t *pCount) {
  if (*pCount == 0)
    *pCount = get_pci_bar_count(device);
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  std::vector<zet_pci_bar_properties_t> PciBarProps(*pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPciGetBars(hSysmanDevice, pCount, PciBarProps.data()));
  return PciBarProps;
}
zet_pci_stats_t get_pci_stats(ze_device_handle_t device) {
  zet_pci_stats_t PciStats;
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanPciGetStats(hSysmanDevice, &PciStats));
  return PciStats;
}
} // namespace level_zero_tests
