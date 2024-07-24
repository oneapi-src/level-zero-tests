/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"

int main(int argc, char **argv) {
  static char sys_env[] = "ZET_ENABLE_PROGRAM_DEBUGGING=1";
  putenv(sys_env);
  static char val_env[] = "ZE_ENABLE_VALIDATION_LAYER=1";
  putenv(val_env);
  static char neg_env[] = "ZE_ENABLE_PARAMETER_VALIDATION=1";
  putenv(neg_env);
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  ze_result_t result = zeInit(2);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();

  return RUN_ALL_TESTS();
}
