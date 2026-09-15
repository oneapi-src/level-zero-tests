/*
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#include "utils_process_helper.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace helper = level_zero_tests::process_helper;
using helper::exit_code;
namespace mode = helper::mode;

int main(int argc, char **argv) {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  if (argc != 2) {
    return helper::to_int(exit_code::missing_mode);
  }
  const std::string selected_mode = argv[1];
  if (selected_mode == mode::slow) {
    std::cout << helper::debug_line << std::flush;
    std::this_thread::sleep_for(helper::slow_initialization_delay);
    std::cout << helper::pass_result;
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::large) {
    for (std::size_t i = 0; i < helper::large_line_count; ++i) {
      std::cout << std::string(helper::large_line_length, 'x') << '\n';
    }
    std::cout << helper::pass_result;
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::error) {
    std::cout << helper::fail_result << helper::fail_reason;
    return helper::to_int(exit_code::initialization_failed);
  }
  if (selected_mode == mode::empty) {
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::environment) {
    const char *value = std::getenv(helper::environment_variable);
    if (!value) {
      return helper::to_int(exit_code::missing_environment);
    }
    std::cout << value << '\n';
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::partial) {
    std::cout << helper::partial_line << std::flush;
  } else if (selected_mode == mode::result) {
    std::cout << helper::pass_result << std::flush;
  } else if (selected_mode == mode::closed) {
    std::cout << helper::pass_result << std::flush;
    std::fclose(stdout);
  } else if (selected_mode != mode::silent &&
             selected_mode != mode::continuous) {
    return helper::to_int(exit_code::unknown_mode);
  }

  for (auto elapsed = decltype(helper::hang_step)::zero();
       elapsed < helper::hang_duration; elapsed += helper::hang_step) {
    if (selected_mode == mode::continuous) {
      std::cout << helper::debug_line << std::flush;
    }
    std::this_thread::sleep_for(helper::hang_step);
  }
  return helper::to_int(exit_code::hang_elapsed);
}
