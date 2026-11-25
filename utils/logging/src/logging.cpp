/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "logging/logging.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "command_line/command_line.hpp"

namespace level_zero_tests {

namespace {

enum class line_format { standard, raw, simple, precise };

const char *const level_names[] = {"trace",   "debug", "info",
                                   "warning", "error", "fatal"};

std::shared_ptr<std::ostream> unowned(std::ostream &stream) {
  return std::shared_ptr<std::ostream>(&stream, [](std::ostream *) {});
}

struct logging_state {
  std::mutex mutex;
  std::atomic<logging_level> min_level{logging_level::trace};
  std::atomic<line_format> format{line_format::standard};
  std::vector<std::shared_ptr<std::ostream>> streams{unowned(std::cout)};
};

logging_state &state() {
  static logging_state *const instance = new logging_state;
  return *instance;
}

void reset_outputs(const line_format format, std::ostream &stream) {
  auto &s = state();
  std::lock_guard<std::mutex> lock(s.mutex);
  s.format.store(format, std::memory_order_relaxed);
  s.streams = {unowned(stream)};
}

std::string timestamp(const std::chrono::system_clock::time_point &now) {
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm local{};
#ifdef _WIN32
  localtime_s(&local, &time);
#else
  localtime_r(&time, &local);
#endif
  char buffer[32];
  return std::string(buffer, std::strftime(buffer, sizeof(buffer),
                                           "%Y-%m-%d %H:%M:%S", &local));
}

const std::string &thread_id() {
  thread_local const std::string id = [] {
    std::ostringstream decimal;
    decimal << std::this_thread::get_id();
    std::ostringstream stream;
    stream << "0x" << std::hex << std::setfill('0') << std::setw(16)
           << std::stoull(decimal.str());
    return stream.str();
  }();
  return id;
}

std::string format_line(const line_format format, const logging_level level,
                        const std::string &message) {
  const std::string severity =
      std::string("[") + level_names[static_cast<size_t>(level)] + "]";
  std::string line;
  switch (format) {
  case line_format::standard: {
    const auto now = std::chrono::system_clock::now();
    const auto microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch())
            .count() %
        1000000;
    char fraction[24];
    std::snprintf(fraction, sizeof(fraction), "%06lld",
                  static_cast<long long>(microseconds));
    line = "[" + timestamp(now) + "." + fraction + "] [" + thread_id() + "] " +
           severity + std::string(9 - severity.size(), ' ') + " " + message;
    break;
  }
  case line_format::raw:
    line = message;
    break;
  case line_format::simple:
    line = severity + " " + message;
    break;
  case line_format::precise:
    line = "[" + timestamp(std::chrono::system_clock::now()) + "] " + severity +
           " " + message;
    break;
  }
  if (line.empty() || line.back() != '\n') {
    line += '\n';
  }
  return line;
}

} // namespace

log_record::log_record(const logging_level level) : level_(level) {
  if (level >= state().min_level.load(std::memory_order_relaxed)) {
    stream_.emplace();
  }
}

void log_record::emit() {
  const std::string message = stream_->str();
  stream_.reset();
  auto &s = state();
  const std::string line =
      format_line(s.format.load(std::memory_order_relaxed), level_, message);
  std::lock_guard<std::mutex> lock(s.mutex);
  for (const auto &stream : s.streams) {
    if (stream->good()) {
      stream->write(line.data(), static_cast<std::streamsize>(line.size()));
      stream->flush();
    }
  }
}

void set_format(const logging_format format) {
  line_format line;
  if (format == logging_format::simple) {
    line = line_format::simple;
  } else if (format == logging_format::precise) {
    line = line_format::precise;
  } else {
    throw std::runtime_error("Unknown logging_format");
  }
  state().format.store(line, std::memory_order_relaxed);
}

void set_min_level(const logging_level level) {
  state().min_level.store(level, std::memory_order_relaxed);
}

void init_logging() { reset_outputs(line_format::raw, std::clog); }

void init_logging(const LoggingSettings settings) {
  init_logging();

  set_format(settings.format);
  set_min_level(settings.level);
}

void init_logging(std::vector<std::string> &command_line) {
  LoggingSettings settings;
  try {
    settings = parse_command_line(command_line);
  } catch (const command_line_error &e) {
    std::cerr << "Error parsing command line: " << e.what() << std::endl;
  }
  try {
    init_logging(settings);
  } catch (std::exception &e) {
    std::cerr << "Error initializing logging: " << e.what() << std::endl;
  }
}

void stop_logging() { reset_outputs(line_format::standard, std::cout); }

void add_stream(const std::shared_ptr<std::ostream> &stream) {
  auto &s = state();
  std::lock_guard<std::mutex> lock(s.mutex);
  s.streams.push_back(stream);
}

std::ostream &operator<<(std::ostream &os, const logging_level &level) {
  const auto index = static_cast<size_t>(level);
  if (index < std::size(level_names)) {
    os << level_names[index];
  } else {
    os << index;
  }
  return os;
}

std::istream &operator>>(std::istream &is, logging_level &level) {
  std::string s;
  is >> s;
  const auto it = std::find(std::begin(level_names), std::end(level_names), s);
  if (it == std::end(level_names)) {
    is.setstate(std::ios_base::failbit);
  } else {
    level = static_cast<logging_level>(it - std::begin(level_names));
  }
  return is;
}

std::ostream &operator<<(std::ostream &os, const logging_format &f) {
  if (f == logging_format::simple) {
    os << "simple";
  } else if (f == logging_format::precise) {
    os << "precise";
  } else {
    throw std::runtime_error("Unknown logging_format");
  }
  return os;
}

std::istream &operator>>(std::istream &is, logging_format &f) {
  std::string s = "";
  is >> s;
  if (s == "simple") {
    f = logging_format::simple;
  } else if (s == "precise") {
    f = logging_format::precise;
  } else {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

LoggingSettings parse_command_line(std::vector<std::string> &command_line) {
  LoggingSettings settings;

  command_line_options options;
  options.value("logging-format", settings.format, "format of logged messages")
      .value("logging-level", settings.level, "minimal logging level to print");
  command_line = options.parse(command_line, true);
  return settings;
}

} // namespace level_zero_tests
