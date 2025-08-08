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

  const std::string var_enable_metrics = "ZET_ENABLE_METRICS";
  const char *env_value = std::getenv(var_enable_metrics.c_str());
  if (env_value != nullptr) {
    LOG_INFO << "ZET_ENABLE_METRICS=1 is Set. Disabling.";
    #if defined(_WIN32) || defined(_WIN64)
    _putenv_s(var_enable_metrics.c_str(), "0");
    #else
    setenv(var_enable_metrics.c_str(), "0", 1) == 0;
    #endif
  }

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";

  LOG_TRACE << "Tools API initialized";
  int returnVal = RUN_ALL_TESTS();

  if (env_value != nullptr) {
    LOG_INFO << "Re-enabling ZET_ENABLE_METRICS=1";
    #if defined(_WIN32) || defined(_WIN64)
    _putenv_s(var_enable_metrics.c_str(), "1");
    #else
    setenv(var_enable_metrics.c_str(), "1", 1) == 0;
    #endif
  }

  return returnVal;
}
