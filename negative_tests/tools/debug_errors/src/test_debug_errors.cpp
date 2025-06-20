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

LZT_TEST(zetDebugAttachErrorsTest,
         GivenInvalidPIDWhenCreatingDebugSessionOnDeviceThenNoSessionCreated) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_invalid_pid_test(devices, false);
}

LZT_TEST(
    zetDebugAttachErrorsTest,
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
class zetDebugDetachErrorsTest : public zetDebugErrorsBaseSetup {};

LZT_TEST_F(
    zetDebugDetachErrorsTest,
    GivenUnConfiguredUnattachedDebuggerWhenCallingDetachThenNotSuccessReturned) {
  zet_debug_session_handle_t debug_session = {};
  EXPECT_NE(ZE_RESULT_SUCCESS, zetDebugDetach(debug_session));
}

} // namespace
