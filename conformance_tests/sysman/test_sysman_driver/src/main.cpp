/*
 *
 * Copyright (C) 2024 Intel Corporation
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

  ze_result_t result = zesInit(0);
  if (result) {
    throw std::runtime_error("zesInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Sysman initialized";
  return RUN_ALL_TESTS();
}
