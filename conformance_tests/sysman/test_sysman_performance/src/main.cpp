/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include <stdlib.h>

int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);
#ifdef USE_ZESINIT
  ze_result_t result = zesInit(0);
  if (result) {
    throw std::runtime_error("zesInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Sysman initialized";
  return RUN_ALL_TESTS();
#else  // USE_ZESINIT
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);
  static char device_hierachy_env[] = "ZE_FLAT_DEVICE_HIERARCHY=COMPOSITE";
  putenv(device_hierachy_env);
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  if (is_sysman_enabled == nullptr) {
    LOG_INFO << "Sysman is not Enabled";
    exit(0);
  } else {
    ze_result_t result = zeInit(1);
    if (result) {
      throw std::runtime_error("zeInit failed: " +
                               level_zero_tests::to_string(result));
    }
    LOG_TRACE << "Driver initialized";

    return RUN_ALL_TESTS();
  }
#endif // USE_ZESINIT
}
