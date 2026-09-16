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
#include <process.h>
#else
#include <unistd.h>
#endif

namespace helper = level_zero_tests::process_helper;
using helper::exit_code;
namespace mode = helper::mode;

int main(int argc, char **argv) {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stderr), _O_BINARY);
#endif
  if (argc != 2 && argc != 3) {
    return helper::to_int(exit_code::missing_mode);
  }
  const std::string selected_mode = argv[1];
  const bool write_stderr =
      argc == 3 && std::string(argv[2]) == helper::stderr_argument;
  if (selected_mode == mode::slow) {
    std::cout << helper::debug_line << std::flush;
    if (write_stderr) {
      std::cerr << helper::stderr_line << std::flush;
    }
    std::this_thread::sleep_for(helper::slow_initialization_delay);
    std::cout << helper::pass_result;
    if (write_stderr) {
      std::cerr << helper::stderr_failure;
      return helper::to_int(exit_code::initialization_failed);
    }
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::large) {
    for (std::size_t i = 0; i < helper::large_line_count; ++i) {
      std::cout << std::string(helper::large_line_length, 'x') << '\n';
      if (write_stderr) {
        std::cerr << std::string(helper::large_line_length, 'E') << std::flush;
      }
    }
    std::cout << helper::pass_result;
    return helper::to_int(exit_code::success);
  }
  if (selected_mode == mode::error) {
    std::cout << helper::fail_result << helper::fail_reason;
    if (write_stderr) {
      std::cerr << helper::stderr_failure;
    }
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
    if (write_stderr) {
      std::cerr << helper::stderr_partial << std::flush;
    }
  } else if (selected_mode == mode::result) {
    std::cout << helper::pass_result << std::flush;
  } else if (selected_mode == mode::closed) {
    std::cout << helper::pass_result << std::flush;
    std::fclose(stdout);
    std::fclose(stderr);
  } else if (selected_mode == mode::stdout_closed) {
    std::fclose(stdout);
    if (write_stderr) {
      std::cerr << helper::stderr_partial << std::flush;
    }
  } else if (selected_mode == mode::stderr_closed) {
    std::fclose(stderr);
    std::cout << helper::partial_line << std::flush;
  } else if (selected_mode == mode::pid) {
#ifdef _WIN32
    std::cout << _getpid() << std::flush;
#else
    std::cout << getpid() << std::flush;
#endif
  } else if (selected_mode != mode::silent &&
             selected_mode != mode::continuous) {
    return helper::to_int(exit_code::unknown_mode);
  }

  for (auto elapsed = decltype(helper::hang_step)::zero();
       elapsed < helper::hang_duration; elapsed += helper::hang_step) {
    if (selected_mode == mode::continuous) {
      std::cout << helper::debug_line << std::flush;
      if (write_stderr) {
        std::cerr << helper::stderr_line << std::flush;
      }
    }
    std::this_thread::sleep_for(helper::hang_step);
  }
  return helper::to_int(exit_code::hang_elapsed);
}
