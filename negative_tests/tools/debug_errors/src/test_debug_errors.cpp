/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_debug.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

void run_invalid_pid_test(std::vector<ze_device_handle_t> &devices,
                          bool use_sub_devices) {

  for (auto &device : devices) {
    if (!zetDebugBaseSetup::is_debug_supported(device))
      continue;

    zet_debug_config_t debug_config = {};

    debug_config.pid = 4000000; // test attaching to a random pid
    zet_debug_session_handle_t debug_session = {};

    LOG_DEBUG << "[Debugger] Attempting to attach to a random pid: "
              << debug_config.pid;

    EXPECT_NE(ZE_RESULT_SUCCESS,
              zetDebugAttach(device, &debug_config, &debug_session));

    if (!debug_session) {
      LOG_DEBUG << "[Debugger] Successfully verified no debug session created";
    } else {
      lzt::debug_detach(debug_session);
      FAIL() << "Debug session created for Invalid PID";
    }
  }
}

TEST(zetDebugAttachErrorsTest,
     GivenInvalidPIDWhenCreatingDebugSessionOnDeviceThenNoSessionCreated) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_invalid_pid_test(devices, false);
}

TEST(zetDebugAttachErrorsTest,
     GivenInvalidPIDWhenCreatingDebugSessionOnSubDeviceThenNoSessionCreated) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_invalid_pid_test(all_sub_devices, true);
}

class zetDebugErrorsBaseSetup : public zetDebugBaseSetup {
protected:
  void SetUp() override {
    zetDebugBaseSetup::SetUp();
    bin_name = "test_debug_errors_helper";
  }
};
class zetDebugDetachErrorsTest : public zetDebugErrorsBaseSetup {
protected:
  void run_detach_error_test(std::vector<ze_device_handle_t> &devices,
                             bool use_sub_devices);
};

void zetDebugDetachErrorsTest::run_detach_error_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {

  for (auto &device : devices) {
    if (!is_debug_supported(device)) {
      continue;
    }

    debugHelper = launch_process(BASIC, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();

    debugSession = lzt::debug_attach(device, debug_config);

    synchro->notify_attach();
    lzt::debug_detach(debugSession);

    EXPECT_NE(ZE_RESULT_SUCCESS, zetDebugDetach(debugSession));

    debugHelper.wait();
  }
}

TEST_F(
    zetDebugDetachErrorsTest,
    GivenUnConfiguredUnattachedDebuggerWhenCallingDetachThenNotSuccessReturned) {
  zet_debug_session_handle_t debug_session = {};
  EXPECT_NE(ZE_RESULT_SUCCESS, zetDebugDetach(debug_session));
}

TEST_F(
    zetDebugDetachErrorsTest,
    GivenDebuggerDetachedFromDeviceApplicationWhenCallingDetachAgainThenNotSuccessReturned) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_detach_error_test(devices, false);
}

TEST_F(
    zetDebugDetachErrorsTest,
    GivenDebuggerDetachedFromSubDeviceApplicationWhenCallingDetachAgainThenNotSuccessReturned) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_detach_error_test(all_sub_devices, true);
}

} // namespace
