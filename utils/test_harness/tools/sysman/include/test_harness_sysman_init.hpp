/*
 *
 * Copyright (C) 2019 Intel Corporation
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

} // namespace level_zero_tests

#endif
