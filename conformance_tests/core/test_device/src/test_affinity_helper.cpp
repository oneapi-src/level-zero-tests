/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

int main(int argc, char **argv) {

  ze_result_t result = zeInit(ZE_INIT_FLAG_NONE);
  if (result != ZE_RESULT_SUCCESS) {
    exit(-1);
  }

  LOG_INFO << "child";
  char *val = getenv("ZE_AFFINITY_MASK");

  for (auto driver : lzt::get_all_driver_handles()) {
    ze_driver_properties_t driver_properties = {};
    driver_properties.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
    driver_properties.pNext = nullptr;

    result = zeDriverGetProperties(driver, &driver_properties);
    if (result != ZE_RESULT_SUCCESS) {
      exit(-1);
    }

    if (!strcmp(argv[1], lzt::to_string(driver_properties.uuid).c_str())) {

      uint32_t num_total_devices = 0;

      auto devices = lzt::get_devices(driver);

      if (devices.empty())
        exit(0);

      for (auto device : devices) {
        auto num_sub_devices = lzt::get_sub_device_count(device);
        num_total_devices += num_sub_devices ? num_sub_devices : 1;
      }

      exit(num_total_devices);
    }
  }

  exit(-1);
}
