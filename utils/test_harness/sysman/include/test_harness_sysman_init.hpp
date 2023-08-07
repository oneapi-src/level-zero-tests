/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_INIT_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_INIT_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"
#include "logging/logging.hpp"

namespace level_zero_tests {

class SysmanCtsClass : public ::testing::Test {
public:
  std::vector<ze_device_handle_t> devices;

  SysmanCtsClass() {
    devices = get_ze_devices();
    if (devices.size() == 0) {
      LOG_INFO << "No device found";
    }
  }
  ~SysmanCtsClass() {}
};

class ZesSysmanCtsClass : public ::testing::Test {
public:
  std::vector<zes_device_handle_t> devices;

  ZesSysmanCtsClass() {
    devices = get_zes_devices();
    if (devices.size() == 0) {
      LOG_INFO << "No device found";
    }
  }
  ~ZesSysmanCtsClass() {}
};

} // namespace level_zero_tests

#endif
