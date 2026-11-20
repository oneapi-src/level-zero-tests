/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "logging/logging.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;

#include "command_line/command_line.hpp"

namespace level_zero_tests {

typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
static boost::shared_ptr<text_sink> sink;

void set_format(const logging_format format) {
  logging::formatter formatter;
  if (format == logging_format::simple) {
    formatter = expr::stream << '[' << logging::trivial::severity << ']' << ' '
                             << expr::smessage;
  } else if (format == logging_format::precise) {
    formatter = expr::stream
                << '['
                << expr::format_date_time<boost::posix_time::ptime>(
                       "TimeStamp", "%Y-%m-%d %H:%M:%S")
                << ']' << ' ' << '[' << logging::trivial::severity << ']' << ' '
                << expr::smessage;
  } else {
    throw std::runtime_error("Unknown logging_format");
  }
  sink->set_formatter(formatter);
}

void set_min_level(const logging_level level) {
  logging::core::get()->set_filter(logging::trivial::severity >= level);
}

void init_logging() {
  sink = logging::add_console_log();
  logging::add_common_attributes();
}

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

void stop_logging() {
  logging::core::get()->remove_sink(sink);
  sink.reset();
}

void add_stream(const boost::shared_ptr<std::ostream> &stream) {
  sink->locked_backend()->add_stream(stream);
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
