/*
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#ifndef LEVEL_ZERO_TESTS_UTILS_PROCESS_HPP
#define LEVEL_ZERO_TESTS_UTILS_PROCESS_HPP

#include <boost/filesystem/path.hpp>
#include <boost/process/v2/process.hpp>
#include <chrono>
#include <string>
#include <vector>

namespace level_zero_tests {

inline constexpr auto default_process_timeout = std::chrono::seconds(120);

void request_process_termination(boost::process::v2::process &process,
                                 boost::system::error_code &ec);
void request_process_termination(boost::process::v2::process &process);

void terminate_process(boost::process::v2::process &process,
                       boost::system::error_code &ec);
void terminate_process(boost::process::v2::process &process);

struct process_output {
  std::string output;
  int exit_code = -1;
  bool timed_out = false;
};

process_output run_process_with_timeout(
    const boost::filesystem::path &executable,
    const std::vector<std::string> &arguments,
    const std::vector<std::string> &environment,
    std::chrono::steady_clock::duration timeout = default_process_timeout);

} // namespace level_zero_tests

#endif
