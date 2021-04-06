/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#define SUCCESS_PREFIX "0"
#define ERROR_PREFIX "1"
#define ERROR_CODE "1:0"

int main(int argc, char **argv) {

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    std::cout << ERROR_CODE << std::endl;
    std::cout << "zeInit failed";
    exit(1);
  }

  char *val = getenv("ZE_AFFINITY_MASK");

  for (auto driver : lzt::get_all_driver_handles()) {
    ze_driver_properties_t driver_properties = {};
    driver_properties.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
    driver_properties.pNext = nullptr;

    result = zeDriverGetProperties(driver, &driver_properties);
    if (result != ZE_RESULT_SUCCESS) {
      std::cout << ERROR_CODE << std::endl;
      std::cout << "zeDriverGetProperties failed";
      exit(1);
    }
    // most significant 32-bits of UUID are timestamp, which can change, so
    // zero-out least significant 32-bits are driver version
    memset(&driver_properties.uuid.id[4], 0, 4);
    if (!strcmp(argv[1], lzt::to_string(driver_properties.uuid).c_str())) {

      uint32_t num_total_devices = 0;

      auto devices = lzt::get_devices(driver);

      for (auto device : devices) {
        auto num_sub_devices = lzt::get_sub_device_count(device);
        num_total_devices += num_sub_devices ? num_sub_devices : 1;
      }

      std::cout << SUCCESS_PREFIX << ":" << num_total_devices << std::endl;
      exit(0);
    }
  }

  std::cout << ERROR_CODE << std::endl;
  std::cout << "Matching driver not found in child process";
  exit(1);
}
