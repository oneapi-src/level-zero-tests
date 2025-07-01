/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

int main(int argc, char **argv) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    std::cout << "zeInit failed";
    exit(1);
  }

  char *val = getenv("ZE_ENABLE_PCI_ID_DEVICE_ORDER");

  for (auto driver : lzt::get_all_driver_handles()) {
    auto devices = lzt::get_devices(driver);
    EXPECT_FALSE(devices.empty());
    std::vector<ze_pci_ext_properties_t> pciPropertiesVec(devices.size());
    auto count = 0;
    for (auto device : devices) {
      ze_pci_ext_properties_t pciProperties{};
      EXPECT_ZE_RESULT_SUCCESS(
          zeDevicePciGetPropertiesExt(device, &pciPropertiesVec[count]));
      std::cout << pciPropertiesVec[count].address.domain << ":"
                << pciPropertiesVec[count].address.bus << ":"
                << pciPropertiesVec[count].address.device << "."
                << pciPropertiesVec[count].address.function << std::endl;
      count++;
    }
    exit(0);
  }

  exit(1);
}
