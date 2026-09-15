/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils_process.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <regex>

#include <sstream>
#include <boost/filesystem.hpp>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace fs = boost::filesystem;

namespace {

static void run_child_process(uint32_t num_devices,
                              std::string device_hierarchy) {
  const auto child_env =
      lzt::child_environment({{"ZE_FLAT_DEVICE_HIERARCHY", device_hierarchy}});

  fs::path helper_path(fs::current_path() / "device");
  fs::path helper = lzt::find_helper_executable("test_device_hierarchy_helper",
                                                {helper_path});

  LOG_INFO << "[Device Hierarchy: " << device_hierarchy << "]";

  const auto child_result =
      lzt::run_process_with_timeout(helper, {}, child_env);
  ASSERT_FALSE(child_result.timed_out)
      << "Timed out waiting for helper output and exit after "
      << lzt::default_process_timeout.count() << " seconds\n"
      << child_result.output;
  std::istringstream child_output(child_result.output);

  int num_devices_child = -1;
  std::string result_string;
  std::getline(child_output, result_string);
  // trim trailing whitespace from result_string
  result_string.erase(std::find_if(result_string.rbegin(), result_string.rend(),
                                   [](int ch) { return !std::isspace(ch); })
                          .base(),
                      result_string.end());

  // ensure that the output matches the expected format:
  while (!std::regex_match(result_string, std::regex("[0-1]:[0-9]+"))) {
    LOG_INFO << result_string;
    if (!std::getline(child_output, result_string)) {
      ADD_FAILURE() << "Error reading from child process: no device count";
      return;
    }
    result_string.erase(std::find_if(result_string.rbegin(),
                                     result_string.rend(),
                                     [](int ch) { return !std::isspace(ch); })
                            .base(),
                        result_string.end());
  }

  LOG_INFO << "Result from Child (RETURN CODE : NUMBER of DEVICES) : "
           << result_string;

  auto result_code =
      std::stoul(result_string.substr(0, result_string.find(":")));
  if (result_code) {
    result_string.clear();
    std::getline(child_output, result_string);
    ADD_FAILURE() << "Child process exited with error getting driver devices: "
                  << result_string;
  } else {
    num_devices_child =
        std::stoi(result_string.substr(result_string.find(":") + 1));
    EXPECT_EQ(num_devices_child, num_devices);
  }
  ASSERT_EQ(child_result.exit_code, 0);
}

LZT_TEST(
    TestDeviceHierarchy,
    GivenDeviceHierarchyFlatThenWhenGettingDevicesThenCorrectNumberOfDevicesReturnedFromRootAndSubDevices) {
  auto device_count = lzt::get_ze_device_count();

  ASSERT_GT(device_count, 0);

  uint32_t total_expected_devices = 0u;
  auto devices = lzt::get_ze_devices(device_count);
  for (auto device : devices) {
    EXPECT_NE(nullptr, device);
    if (lzt::get_sub_device_count(device) > 0) {
      total_expected_devices += lzt::get_sub_device_count(device);
    } else {
      total_expected_devices++;
    }
  }
  run_child_process(total_expected_devices, "FLAT");
}

LZT_TEST(
    TestDeviceHierarchy,
    GivenDeviceHierarchyCombinedThenWhenGettingDevicesThenCorrectNumberOfDevicesReturnedFromRootAndSubDevices) {
  auto device_count = lzt::get_ze_device_count();

  ASSERT_GT(device_count, 0);

  uint32_t total_expected_devices = 0u;
  auto devices = lzt::get_ze_devices(device_count);
  for (auto device : devices) {
    EXPECT_NE(nullptr, device);
    if (lzt::get_sub_device_count(device) > 0) {
      total_expected_devices += lzt::get_sub_device_count(device);
    } else {
      total_expected_devices++;
    }
  }
  run_child_process(total_expected_devices, "COMBINED");
}

LZT_TEST(
    TestDeviceHierarchy,
    GivenDeviceHierarchyCompositeThenWhenGettingDevicesThenCorrectNumberOfDevicesReturnedFromRootAndSubDevices) {
  auto device_count = lzt::get_ze_device_count();

  ASSERT_GT(device_count, 0);

  uint32_t total_expected_devices = 0u;
  auto devices = lzt::get_ze_devices(device_count);
  for (auto device : devices) {
    EXPECT_NE(nullptr, device);
    total_expected_devices++;
  }
  run_child_process(total_expected_devices, "COMPOSITE");
}

} // namespace
