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
  const auto env_value = level_zero_tests::getenv(var_enable_metrics);
  if (env_value) {
    LOG_INFO << "ZET_ENABLE_METRICS=1 is Set. Disabling.";
    level_zero_tests::putenv(var_enable_metrics, "0");
  }

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_ERROR << "zeInit failed: " << level_zero_tests::to_string(result);
    return EXIT_FAILURE;
  }
  LOG_TRACE << "Driver initialized";

  LOG_TRACE << "Tools API initialized";
  int return_val = RUN_ALL_TESTS();

  if (env_value) {
    LOG_INFO << "Re-enabling ZET_ENABLE_METRICS=1";
    level_zero_tests::putenv(var_enable_metrics, "1");
  }

  return return_val;
}
