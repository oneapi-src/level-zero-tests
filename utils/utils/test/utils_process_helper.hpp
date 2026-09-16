/*
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef LEVEL_ZERO_TESTS_UTILS_PROCESS_HELPER_HPP
#define LEVEL_ZERO_TESTS_UTILS_PROCESS_HELPER_HPP

#include <chrono>
#include <cstddef>
#include <string_view>

namespace level_zero_tests {
namespace process_helper {

namespace mode {
inline constexpr const char *slow = "slow";
inline constexpr const char *large = "large";
inline constexpr const char *error = "error";
inline constexpr const char *empty = "empty";
inline constexpr const char *environment = "environment";
inline constexpr const char *partial = "partial";
inline constexpr const char *result = "result";
inline constexpr const char *closed = "closed";
inline constexpr const char *silent = "silent";
inline constexpr const char *continuous = "continuous";
inline constexpr const char *stdout_closed = "stdout_closed";
inline constexpr const char *stderr_closed = "stderr_closed";
inline constexpr const char *pid = "pid";
} // namespace mode

enum class exit_code : int {
  success = 0,
  missing_mode = 2,
  missing_environment = 3,
  unknown_mode = 4,
  hang_elapsed = 5,
  initialization_failed = 7,
};

constexpr int to_int(exit_code code) { return static_cast<int>(code); }

inline constexpr const char *environment_variable = "LZT_PROCESS_TEST_VALUE";

inline constexpr std::string_view pass_result = "0:1\n";
inline constexpr std::string_view fail_result = "1:0\n";
inline constexpr std::string_view fail_reason = "zeInit failed";
inline constexpr std::string_view debug_line = "debug settings\n";
inline constexpr std::string_view partial_line = "partial line";
inline constexpr const char *stderr_argument = "stderr";
inline constexpr std::string_view stderr_line = "stderr diagnostics\n";
inline constexpr std::string_view stderr_failure = "helper failed";
inline constexpr std::string_view stderr_partial = "partial stderr";

// "large" writes this many newline-terminated lines to overflow the pipe
// capacity before the final result line.
inline constexpr std::size_t large_line_count = 4096;
inline constexpr std::size_t large_line_length = 128;
inline constexpr std::size_t large_output_size =
    large_line_count * (large_line_length + 1) + pass_result.size();

// "slow" withholds its result past the five second mark, which the timeout
// must not treat as a hang.
inline constexpr auto slow_initialization_delay = std::chrono::seconds(6);

// Hang modes stay alive far longer than any test timeout, waking up often
// enough to keep "continuous" writing.
inline constexpr auto hang_duration = std::chrono::seconds(30);
inline constexpr auto hang_step = std::chrono::milliseconds(10);

} // namespace process_helper
} // namespace level_zero_tests

#endif
