/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace level_zero_tests {

enum class logging_level { trace, debug, info, warning, error, fatal };
std::ostream &operator<<(std::ostream &os, const logging_level &level);
std::istream &operator>>(std::istream &is, logging_level &level);

class log_record {
public:
  explicit log_record(logging_level level);
  log_record(const log_record &) = delete;
  log_record &operator=(const log_record &) = delete;

  bool pending() const { return stream_.has_value(); }
  std::ostream &stream() { return *stream_; }
  void emit();

private:
  logging_level level_;
  std::optional<std::ostringstream> stream_;
};

#define LZT_LOG(level)                                                         \
  for (::level_zero_tests::log_record lzt_log_record(                          \
           ::level_zero_tests::logging_level::level);                          \
       lzt_log_record.pending(); lzt_log_record.emit())                        \
  lzt_log_record.stream()

#define LOG_TRACE LZT_LOG(trace)
#define LOG_DEBUG LZT_LOG(debug)
#define LOG_INFO LZT_LOG(info)
#define LOG_WARNING LZT_LOG(warning)
#define LOG_ERROR LZT_LOG(error)
#define LOG_FATAL LZT_LOG(fatal)

#define LOG_ENTER_FUNCTION LOG_TRACE << "Enter function: " << __func__;
#define LOG_EXIT_FUNCTION LOG_TRACE << "Exit function: " << __func__;

enum class logging_format { simple, precise };
std::ostream &operator<<(std::ostream &os, const logging_format &f);
std::istream &operator>>(std::istream &is, logging_format &f);

struct LoggingSettings {
  logging_format format = logging_format::precise;
  logging_level level = logging_level::info;
};

void init_logging();
void init_logging(const LoggingSettings settings);
void init_logging(std::vector<std::string> &command_line);
void stop_logging();
void add_stream(const std::shared_ptr<std::ostream> &stream);
LoggingSettings parse_command_line(std::vector<std::string> &command_line);

template <typename T> std::string to_string(const std::vector<T> &x) {
  std::stringstream ss;
  ss << '[';
  if (!x.empty()) {
    std::copy(x.begin(), x.end() - 1, std::ostream_iterator<T>(ss, ", "));
    ss << x.back();
  }
  ss << ']';
  return ss.str();
}

} // namespace level_zero_tests
