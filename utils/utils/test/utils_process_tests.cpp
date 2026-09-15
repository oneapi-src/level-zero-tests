/*
 * Copyright (C) 2026 Intel Corporation
 * SPDX-License-Identifier: MIT
 */

#include "utils/utils.hpp"
#include "utils/utils_process.hpp"
#include "utils_process_helper.hpp"
#include "gtest/gtest.h"

#include <boost/process.hpp>
#include <boost/process/v2/ext/exe.hpp>
#include <chrono>

#ifdef _WIN32
#include <boost/process/v2/windows/creation_flags.hpp>
#include <windows.h>
#endif

namespace lzt = level_zero_tests;
namespace bp = boost::process::v2;
namespace helper = level_zero_tests::process_helper;
namespace mode = helper::mode;
using helper::exit_code;
using namespace std::chrono_literals;

namespace {

boost::filesystem::path helper_path() {
  return lzt::find_helper_executable(
      "utils_process_test_helper",
      {bp::ext::exe(bp::current_pid()).parent_path()});
}

lzt::process_output run_helper(const std::string &mode,
                               std::chrono::steady_clock::duration timeout) {
  return lzt::run_process_with_timeout(helper_path(), {mode},
                                       lzt::child_environment({}), timeout);
}

LZT_TEST(ProcessTimeout, AllowsInitializationLongerThanFiveSeconds) {
  const auto result = run_helper(mode::slow, 15s);
  EXPECT_FALSE(result.timed_out);
  EXPECT_EQ(result.exit_code, helper::to_int(exit_code::success));
  EXPECT_EQ(result.output,
            std::string(helper::debug_line).append(helper::pass_result));
}

LZT_TEST(ProcessTimeout, DrainsOutputLargerThanPipeCapacity) {
  const auto result = run_helper(mode::large, 5s);
  EXPECT_FALSE(result.timed_out);
  EXPECT_EQ(result.exit_code, helper::to_int(exit_code::success));
  ASSERT_EQ(result.output.size(), helper::large_output_size);
  EXPECT_EQ(
      result.output.substr(result.output.size() - helper::pass_result.size()),
      helper::pass_result);
}

LZT_TEST(ProcessTimeout, PreservesFailureAndUnterminatedFinalLine) {
  const auto result = run_helper(mode::error, 5s);
  EXPECT_FALSE(result.timed_out);
  EXPECT_EQ(result.exit_code, helper::to_int(exit_code::initialization_failed));
  EXPECT_EQ(result.output,
            std::string(helper::fail_result).append(helper::fail_reason));
}

LZT_TEST(ProcessTimeout, AcceptsEmptyOutput) {
  const auto result = run_helper(mode::empty, 5s);
  EXPECT_FALSE(result.timed_out);
  EXPECT_EQ(result.exit_code, helper::to_int(exit_code::success));
  EXPECT_TRUE(result.output.empty());
}

LZT_TEST(ProcessTimeout, PassesChildEnvironment) {
  const auto result = lzt::run_process_with_timeout(
      helper_path(), {mode::environment},
      lzt::child_environment({{helper::environment_variable, "override"}}), 5s);
  EXPECT_FALSE(result.timed_out);
  EXPECT_EQ(result.exit_code, helper::to_int(exit_code::success));
  EXPECT_EQ(result.output, "override\n");
}

LZT_TEST(ProcessTermination, AcceptsUnlaunchedProcess) {
  boost::asio::io_context io;
  bp::process child(io);
  EXPECT_NO_THROW(lzt::request_process_termination(child));
  EXPECT_NO_THROW(lzt::terminate_process(child));
}

LZT_TEST(ProcessTermination, PreservesNormalExitAndAllowsRepeatedCleanup) {
  boost::asio::io_context io;
  bp::process child(io, helper_path(), {mode::empty});
  child.wait();
  ASSERT_EQ(child.exit_code(), helper::to_int(exit_code::success));
  EXPECT_NO_THROW(lzt::request_process_termination(child));
  EXPECT_NO_THROW(lzt::terminate_process(child));
  EXPECT_NO_THROW(lzt::terminate_process(child));
  EXPECT_EQ(child.exit_code(), helper::to_int(exit_code::success));
}

LZT_TEST(ProcessTermination, KillsAndReapsLiveProcess) {
  boost::asio::io_context io;
  bp::process child(io, helper_path(), {mode::silent});
  lzt::terminate_process(child);
  EXPECT_FALSE(child.running());
  EXPECT_NE(child.exit_code(), helper::to_int(exit_code::success));
  EXPECT_NO_THROW(lzt::terminate_process(child));
}

LZT_TEST(ProcessTermination, RequestLeavesHandleAvailableForExplicitWait) {
  boost::asio::io_context io;
  bp::process child(io, helper_path(), {mode::silent});
  lzt::request_process_termination(child);
  EXPECT_TRUE(child.is_open());
  child.wait();
  EXPECT_FALSE(child.running());
  EXPECT_NE(child.exit_code(), helper::to_int(exit_code::success));
}

#ifdef _WIN32
LZT_TEST(ProcessTermination, AcceptsNativeExitNotYetObservedByBoost) {
  boost::asio::io_context io;
  bp::process child(io, helper_path(), {mode::empty});
  ASSERT_EQ(WaitForSingleObject(child.native_handle(), 5000), WAIT_OBJECT_0);
  EXPECT_NO_THROW(lzt::terminate_process(child));
  EXPECT_EQ(child.exit_code(), helper::to_int(exit_code::success));
}

LZT_TEST(ProcessTermination, PreservesAccessDeniedForLiveProcess) {
  boost::asio::io_context io;
  bp::process child(io, helper_path(), {mode::silent});
  HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                              FALSE, child.id());
  ASSERT_NE(handle, nullptr);
  bp::process restricted(io.get_executor(), child.id(), handle);
  boost::system::error_code ec;
  lzt::request_process_termination(restricted, ec);
  EXPECT_EQ(ec.value(), ERROR_ACCESS_DENIED);
  EXPECT_TRUE(child.running());
  lzt::terminate_process(child);
}

LZT_TEST(ProcessTermination, RequestReturnsWithPendingWindowsDebugEvent) {
  boost::asio::io_context io;
  bp::process child(
      io, helper_path(), {mode::silent},
      bp::windows::process_creation_flags<DEBUG_ONLY_THIS_PROCESS>{});
  // Detach before the process destructor on assertion failure as well.
  struct debug_guard {
    DWORD pid;
    ~debug_guard() { DebugActiveProcessStop(pid); }
  } guard{child.id()};

  DEBUG_EVENT event{};
  ASSERT_TRUE(WaitForDebugEvent(&event, 5000));
  ASSERT_EQ(event.dwDebugEventCode, CREATE_PROCESS_DEBUG_EVENT);
  if (event.u.CreateProcessInfo.hFile) {
    CloseHandle(event.u.CreateProcessInfo.hFile);
  }

  // A blocking terminate() cannot return until the pending event is released.
  lzt::request_process_termination(child);
  ASSERT_TRUE(
      ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE));
  do {
    ASSERT_TRUE(WaitForDebugEvent(&event, 5000));
    if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT &&
        event.u.LoadDll.hFile) {
      CloseHandle(event.u.LoadDll.hFile);
    }
    ASSERT_TRUE(
        ContinueDebugEvent(event.dwProcessId, event.dwThreadId, DBG_CONTINUE));
  } while (event.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);
  child.wait();
  EXPECT_NE(child.exit_code(), helper::to_int(exit_code::success));
}
#endif

class HungProcess : public testing::TestWithParam<const char *> {};

LZT_TEST_P(HungProcess, TerminatesAndReturnsWithinDeadline) {
  const auto start = std::chrono::steady_clock::now();
  const auto result = run_helper(GetParam(), 1s);
  const auto elapsed = std::chrono::steady_clock::now() - start;
  EXPECT_TRUE(result.timed_out);
  EXPECT_NE(result.exit_code, helper::to_int(exit_code::success));
  EXPECT_LT(elapsed, 5s);
  const std::string selected_mode = GetParam();
  if (selected_mode == mode::partial) {
    EXPECT_EQ(result.output, helper::partial_line);
  } else if (selected_mode == mode::result || selected_mode == mode::closed) {
    EXPECT_EQ(result.output, helper::pass_result);
  } else if (selected_mode == mode::continuous) {
    EXPECT_NE(result.output.find(helper::debug_line), std::string::npos);
  } else {
    EXPECT_TRUE(result.output.empty());
  }
}

INSTANTIATE_TEST_SUITE_P(ProcessTimeout, HungProcess,
                         testing::Values(mode::silent, mode::partial,
                                         mode::result, mode::closed,
                                         mode::continuous));

} // namespace
