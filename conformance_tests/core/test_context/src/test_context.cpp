/*
 *
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <string.h>
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace {

class ContextExCreateTests : public ::testing::Test {
protected:
  ContextExCreateTests() {
    driver = lzt::get_default_driver();
    devices = lzt::get_devices(driver);
  }

  void run_test(ze_context_handle_t context,
                std::vector<ze_device_handle_t> devices, bool is_immediate) {

    const size_t buff_size = 256;
    auto buffer = lzt::allocate_host_memory(buff_size, 1, context);
    auto ref_buffer = new uint8_t[buff_size];
    uint8_t val = 0x55;
    memset(ref_buffer, val, buff_size);

    for (auto device : devices) {
      memset(buffer, 0, buff_size);
      auto bundle = lzt::create_command_bundle(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
      lzt::append_memory_set(bundle.list, buffer, &val, buff_size);
      lzt::close_command_list(bundle.list);
      lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

      EXPECT_EQ(memcmp(ref_buffer, buffer, buff_size), 0);
      lzt::destroy_command_bundle(bundle);
      if (::testing::Test::HasFailure()) {
        break;
      }
    }

    lzt::free_memory(context, buffer);
    delete[] ref_buffer;
  }

  ze_driver_handle_t driver;
  std::vector<ze_device_handle_t> devices;
};

LZT_TEST_F(ContextExCreateTests,
           GivenContextOnAllDevicesWhenUsingContextThenSuccess) {
  auto context = lzt::create_context_ex(driver);
  run_test(context, devices, false);
}

LZT_TEST_F(
    ContextExCreateTests,
    GivenContextOnAllDevicesWhenUsingContextWithImmediateCmdListThenSuccess) {
  auto context = lzt::create_context_ex(driver);
  run_test(context, devices, true);
}

LZT_TEST_F(ContextExCreateTests,
           GivenContextOnMultipleDevicesWhenUsingContextThenSuccess) {
  // This test requires two or more devices
  if (devices.size() < 2) {
    GTEST_SKIP() << "Less than two devices, skipping test";
  }

  // remove an element so test is creating context on only some devices.
  devices.pop_back();
  auto context = lzt::create_context_ex(driver, devices);
  run_test(context, devices, false);
}

LZT_TEST_F(
    ContextExCreateTests,
    GivenContextOnMultipleDevicesWhenUsingContextWithImmediateCmdListThenSuccess) {
  // This test requires two or more devices
  if (devices.size() < 2) {
    GTEST_SKIP() << "Less than two devices, skipping test";
  }

  // remove an element so test is creating context on only some devices.
  devices.pop_back();
  auto context = lzt::create_context_ex(driver, devices);
  run_test(context, devices, true);
}

LZT_TEST(
    ContextStatusTest,
    GivenContextCreateWhenUsingValidHandleThenContextGetStatusReturnsSuccess) {
  auto context = lzt::get_default_context();
  ASSERT_ZE_RESULT_SUCCESS(zeContextGetStatus(context));
}

} // namespace
