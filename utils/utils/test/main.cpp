/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "logging/logging.hpp"

int main(int argc, char **argv) {
  try {
    ::testing::InitGoogleTest(&argc, argv);
  } catch (std::exception &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Test failed with unknown exception" << std::endl;
    return 1;
  }
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  try {
    auto result = RUN_ALL_TESTS();
    return result;
  } catch (std::runtime_error &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
