/*
 *
 * Copyright (C) 2021 Intel Corporation
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
                std::vector<ze_device_handle_t> devices) {

    const size_t buff_size = 256;
    auto buffer = lzt::allocate_host_memory(buff_size, 1, context);
    auto ref_buffer = new uint8_t[buff_size];
    uint8_t val = 0x55;
    memset(ref_buffer, val, buff_size);

    for (auto device : devices) {
      memset(buffer, 0, buff_size);
      auto cmdlist = lzt::create_command_list(context, device, 0);
      auto cmdqueue = lzt::create_command_queue(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
      lzt::append_memory_set(cmdlist, buffer, &val, buff_size);
      lzt::close_command_list(cmdlist);
      lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
      lzt::synchronize(cmdqueue, UINT64_MAX);

      ASSERT_EQ(memcmp(ref_buffer, buffer, buff_size), 0);
      lzt::destroy_command_list(cmdlist);
      lzt::destroy_command_queue(cmdqueue);
    }

    lzt::free_memory(buffer);
    delete[] ref_buffer;
  }

  ze_driver_handle_t driver;
  std::vector<ze_device_handle_t> devices;
};

TEST_F(ContextExCreateTests,
       GivenContextOnAllDevicesWhenUsingContextThenSuccess) {
  auto context = lzt::create_context_ex(driver);
  run_test(context, devices);
}

TEST_F(ContextExCreateTests,
       GivenContextOnSomeDevicesWhenUsingContextThenSuccess) {
  // This test requires two or more devices
  if (devices.size() < 2) {
    LOG_WARNING << "Less than two devices, skipping test";
    return;
  }

  // remove an element so test is creating context on only some devices.
  devices.pop_back();
  auto context = lzt::create_context_ex(driver, devices);
  run_test(context, devices);
}

} // namespace
