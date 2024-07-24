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
#include <level_zero/loader/ze_loader.h>

int main(int argc, char **argv) {
  #ifndef USE_RUNTIME_TRACING
  static char tracing_env[] = "ZE_ENABLE_TRACING_LAYER=1";
  putenv(tracing_env);
  #endif
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  ze_result_t result = zeInit(2);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  #ifdef USE_RUNTIME_TRACING
  zelEnableTracingLayer();
  #endif
  LOG_TRACE << "Driver initialized";

  auto ret = RUN_ALL_TESTS();
  #ifdef USE_RUNTIME_TRACING
  zelDisableTracingLayer();
  #endif
  return ret;
}
