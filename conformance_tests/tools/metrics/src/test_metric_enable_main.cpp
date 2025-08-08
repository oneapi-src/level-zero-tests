/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"

int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  const std::string varEnableMetrics = "ZET_ENABLE_METRICS";
  const char *envValue = std::getenv(varEnableMetrics.c_str());
  if (envValue != nullptr) {
    LOG_INFO << "ZET_ENABLE_METRICS=1 is Set. Disabling.";
    setenv(varEnableMetrics.c_str(), "0", 1) == 0;
  }

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";

  LOG_TRACE << "Tools API initialized";
  int returnVal = RUN_ALL_TESTS();

  if (envValue != nullptr) {
    LOG_INFO << "Re-enabling ZET_ENABLE_METRICS=1";
    setenv(varEnableMetrics.c_str(), "1", 1) == 0;
  }
  return returnVal;
}
