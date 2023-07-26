/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <regex>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace bp = boost::process;
namespace fs = boost::filesystem;

namespace {

std::string get_result(bp::ipstream &stream) {

  std::string result;
  std::getline(stream, result);

  return result;
}

static void run_child_process(uint32_t num_devices,
                              std::string device_hierarchy) {
  auto env = boost::this_process::environment();
  bp::environment child_env = env;
  child_env["ZE_FLAT_DEVICE_HIERARCHY"] = device_hierarchy;

  fs::path helper_path(boost::filesystem::current_path() / "device");
  std::vector<boost::filesystem::path> paths;
  paths.push_back(helper_path);
  bp::ipstream child_output;
  fs::path helper = bp::search_path("test_device_hierarchy_helper", paths);
  bp::child get_devices_process(helper, child_env, bp::std_out > child_output);
  get_devices_process.wait();

  LOG_INFO << "[Device Hierarchy: " << device_hierarchy << "]";

  ASSERT_EQ(get_devices_process.exit_code(), 0);

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
    // spawn thread to read the next line if available
    auto future =
        std::async(std::launch::async, get_result, std::ref(child_output));

    auto status = future.wait_until(std::chrono::system_clock::now() +
                                    std::chrono::seconds(5));

    if (status == std::future_status::ready) {
      result_string = future.get();
      result_string.erase(std::find_if(result_string.rbegin(),
                                       result_string.rend(),
                                       [](int ch) { return !std::isspace(ch); })
                              .base(),
                          result_string.end());
      if (result_string.empty()) {
        ADD_FAILURE() << "Error reading from child process";
        return;
      }
    } else {
      ADD_FAILURE() << "Timed out waiting for expected result format";
      return;
    }
  }

  LOG_INFO << "Result from Child (RETURN CODE : NUMBER of DEVICES) : "
           << result_string;

  auto result_code =
      std::stoul(result_string.substr(0, result_string.find(":")));
  if (result_code) {
    std::getline(child_output, result_string);
    ADD_FAILURE() << "Child process exited with error getting driver devices: "
                  << result_string;
  } else {
    num_devices_child =
        std::stoul(result_string.substr(result_string.find(":") + 1));
    EXPECT_EQ(num_devices_child, num_devices);
  }
}

TEST(
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

TEST(
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

TEST(
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
