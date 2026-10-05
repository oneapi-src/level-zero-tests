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

  const auto device_hierarchy = lzt::getenv("ZE_FLAT_DEVICE_HIERARCHY");

  for (auto driver : lzt::get_all_driver_handles()) {
    auto devices = lzt::get_devices(driver);
    EXPECT_FALSE(devices.empty());
    for (auto device : devices) {
      if (device_hierarchy) {
        auto device_props = lzt::get_device_properties(device);
        auto sub_device_count = lzt::get_sub_device_count(device);
        auto root_device = lzt::get_root_device(device);
        if (device_hierarchy == "FLAT") {
          EXPECT_FALSE(device_props.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE);
          EXPECT_EQ(sub_device_count, 0u);
          EXPECT_EQ(root_device, nullptr);
        } else if (device_hierarchy == "COMBINED") {
          if (root_device) {
            EXPECT_GT(lzt::get_sub_device_count(root_device), 0u);
            auto sub_devices = lzt::get_ze_sub_devices(root_device);
            for (auto sub_device : sub_devices) {
              auto root_device_handle = lzt::get_root_device(sub_device);
              EXPECT_EQ(root_device, root_device_handle);
            }
          }
        } else if (device_hierarchy == "COMPOSITE") {
          if (sub_device_count > 0) {
            auto sub_devices = lzt::get_ze_sub_devices(device);
            for (auto sub_device : sub_devices) {
              auto root_device_handle = lzt::get_root_device(sub_device);
              EXPECT_EQ(device, root_device_handle);
            }
          }
        }
      }
    }
    std::cout << "0"
              << ":" << devices.size() << std::endl;
    exit(0);
  }
  std::cout << "1:0" << std::endl;
  exit(1);
}
