/*
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#include "utils/utils_process.hpp"

#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/process.hpp>

#ifdef _WIN32
#include <boost/winapi/get_last_error.hpp>
#include <boost/winapi/process.hpp>
#else
#include <cerrno>
#include <signal.h>
#endif

namespace level_zero_tests {

void request_process_termination(boost::process::v2::process &process,
                                 boost::system::error_code &ec) {
  ec.clear();
  if (!process.is_open() || !process.running(ec) || ec) {
    return;
  }

#ifdef _WIN32
  if (!boost::winapi::TerminateProcess(process.native_handle(), 1)) {
    ec.assign(static_cast<int>(boost::winapi::GetLastError()),
              boost::system::system_category());
  }
#else
  if (::kill(process.id(), SIGKILL) == -1) {
    ec.assign(errno, boost::system::system_category());
  }
#endif
  if (ec) {
    boost::system::error_code status_error;
    if (!process.running(status_error) && !status_error) {
      ec.clear();
    }
  }
}

void request_process_termination(boost::process::v2::process &process) {
  boost::system::error_code ec;
  request_process_termination(process, ec);
  if (ec) {
    throw boost::system::system_error(ec, "Requesting helper termination");
  }
}

void terminate_process(boost::process::v2::process &process,
                       boost::system::error_code &ec) {
  request_process_termination(process, ec);
  if (!ec && process.is_open()) {
    process.wait(ec);
  }
}

void terminate_process(boost::process::v2::process &process) {
  boost::system::error_code ec;
  terminate_process(process, ec);
  if (ec) {
    throw boost::system::system_error(ec, "Terminating helper");
  }
}

process_output
run_process_with_timeout(const boost::filesystem::path &executable,
                         const std::vector<std::string> &arguments,
                         const std::vector<std::string> &environment,
                         std::chrono::steady_clock::duration timeout) {
  namespace bp = boost::process::v2;
  namespace asio = boost::asio;

  asio::io_context io;
  asio::steady_timer deadline(io);
  deadline.expires_after(timeout);
  bp::popen child(io, executable, arguments,
                  bp::process_environment{environment});
  asio::cancellation_signal cancel_wait;
  process_output result;
  bool read_done = false;
  bool wait_done = false;
  boost::system::error_code read_error, wait_error;

  const auto cancel_operations = [&] {
    boost::system::error_code ignored;
    child.get_stdout().close(ignored);
    cancel_wait.emit(asio::cancellation_type::all);
  };

  asio::async_read(child, asio::dynamic_buffer(result.output),
                   [&](boost::system::error_code ec, std::size_t) {
                     read_done = true;
                     if (!result.timed_out && ec != asio::error::eof) {
                       read_error = ec;
                     }
                     if (read_error) {
                       cancel_operations();
                     }
                     if (read_error || wait_done) {
                       deadline.cancel();
                     }
                   });
  child.async_wait(asio::bind_cancellation_slot(
      cancel_wait.slot(), [&](boost::system::error_code ec, int exit_code) {
        wait_done = !ec;
        result.exit_code = exit_code;
        if (!result.timed_out) {
          wait_error = ec;
        }
        if (ec) {
          boost::system::error_code ignored;
          child.get_stdout().close(ignored);
        }
        if (ec || read_done) {
          deadline.cancel();
        }
      }));
  deadline.async_wait([&](boost::system::error_code ec) {
    if (!ec) {
      result.timed_out = true;
      cancel_operations();
    }
  });
  io.run();

  if (!wait_done) {
    terminate_process(child);
    result.exit_code = child.exit_code();
  }
  if (read_error) {
    throw boost::system::system_error(read_error, "Reading helper stdout");
  }
  if (wait_error) {
    throw boost::system::system_error(wait_error, "Waiting for helper exit");
  }
  return result;
}

} // namespace level_zero_tests
