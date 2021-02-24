/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include <stdlib.h>

int main(int argc, char **argv) {
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);
  auto isSysmanEnabled = getenv("ZES_ENABLE_SYSMAN");
  if (isSysmanEnabled == nullptr) {
    LOG_INFO << "Sysman is not Enabled";
    exit(0);
  } else {
    auto isSysmanEnabledInt = atoi(isSysmanEnabled);
    if (isSysmanEnabledInt == 1) {
      ze_result_t result = zeInit(0);
      if (result) {
        throw std::runtime_error("zeInit failed: " +
                                 level_zero_tests::to_string(result));
      }
      LOG_TRACE << "Driver initialized";

      return RUN_ALL_TESTS();
    } else {
      LOG_INFO << "Sysman is not Enabled";
      exit(0);
    }
  }
}
