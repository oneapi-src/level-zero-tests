/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/process.hpp>
#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace bp = boost::process;
namespace fs = boost::filesystem;

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

static void run_child_process(std::string driver_id, std::string affinity_mask,
                              uint32_t num_devices_mask) {
  auto env = boost::this_process::environment();
  bp::environment child_env = env;
  child_env["ZE_AFFINITY_MASK"] = affinity_mask;

  fs::path helper_path(boost::filesystem::current_path() / "device");
  std::vector<boost::filesystem::path> paths;
  paths.push_back(helper_path);
  bp::ipstream child_output;
  fs::path helper = bp::search_path("test_affinity_helper", paths);
  bp::child get_devices_process(helper, driver_id, child_env,
                                bp::std_out > child_output);
  get_devices_process.wait();

  int num_devices_child = -1;
  std::string result_string;
  std::getline(child_output, result_string);

  LOG_INFO << "[Affinity Mask: " << affinity_mask << "]";
  LOG_INFO << "Result from Child: " << result_string;

  auto result_code =
      std::stoul(result_string.substr(0, result_string.find(":")));
  if (result_code) {
    std::getline(child_output, result_string);
    ADD_FAILURE() << "Child process exited with error getting driver devices: "
                  << result_string;
  } else {
    num_devices_child =
        std::stoul(result_string.substr(result_string.find(":") + 1));
    EXPECT_EQ(num_devices_child, num_devices_mask);
  }
}

TEST(
    TestAffinity,
    GivenAffinityMaskSetWhenGettingDevicesThenCorrectNumberOfDevicesReturnedInAllCases) {

  std::string affinity_mask = "";

  for (auto driver : lzt::get_all_driver_handles()) {
    auto devices = lzt::get_devices(driver);
    uint32_t num_total_devices = 0;
    if (devices.empty()) {
      continue;
    }
    for (auto device : devices) {
      auto num_sub_devices = lzt::get_sub_device_count(device);
      num_total_devices += num_sub_devices ? num_sub_devices : 1;
    }

    // in each mask, either a device is present or it is not,
    // so for N devices there are 2^N possible masks
    auto num_masks = 1 << num_total_devices;
    for (uint32_t mask = 0; mask < num_masks; mask++) {
      uint32_t num_devices_mask = 0;
      auto temp = mask;
      while (temp) {
        num_devices_mask += temp & 0x1;
        temp >>= 1;
      }
      // an unset mask means all devices should be returned
      if (num_devices_mask == 0) {
        num_devices_mask = num_total_devices;
      }

      // build this affinity mask string
      std::stringstream os;
      auto temp_mask = mask;
      for (int root_device = 0; root_device < devices.size(); root_device++) {
        uint32_t num_sub_devices_in_mask = 0;
        std::vector<int> sub_devices;
        auto num_sub_devices = lzt::get_sub_device_count(devices[root_device]);

        for (int i = 0; i < num_sub_devices; i++) {
          if (temp_mask & 0x01) { // include this subdevice?
            num_sub_devices_in_mask++;
            // save this subdevice
            sub_devices.push_back(i);
          }
          temp_mask >>= 1;
        }

        if (num_sub_devices == 0) {
          if (os.rdbuf()->in_avail()) {
            os << ",";
          }
          os << ((temp_mask & 0x1) ? std::to_string(root_device) : "");
          temp_mask >>= 1;
        } else if (!num_sub_devices_in_mask) { // no subdevices specified, don't
                                               // include this device
          os << "";
        } else if (num_sub_devices_in_mask ==
                   num_sub_devices) { // all subdevices specified
          if (os.rdbuf()->in_avail()) {
            os << ",";
          }
          os << root_device;
        } else { // specific devices should be listed
          if (os.rdbuf()->in_avail()) {
            os << ",";
          }
          for (int j = 0; j < sub_devices.size() - 1; j++) {
            os << root_device << "." << sub_devices[j];
            os << ",";
          }
          os << root_device << "." << sub_devices[sub_devices.size() - 1];
        }
      }
      affinity_mask = os.str();

      // launch child process with constructed mask
      auto driver_properties = lzt::get_driver_properties(driver);
      run_child_process(lzt::to_string(driver_properties.uuid), affinity_mask,
                        num_devices_mask);
    }
  }
}

TEST(TestAffinity,
     GivenAffinityMaskSetWhenGettingDevicesThenCorrectNumberOfDevicesReturned) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (devices.size() < 2) {
    LOG_WARNING << "Test not executed due to not enough devices";
    return;
  }

  uint32_t num_total_devices = 0;
  for (int i = 0; i < devices.size(); i++) {
    auto num_sub_devices = lzt::get_sub_device_count(devices[i]);
    num_total_devices += num_sub_devices ? num_sub_devices : 1;
  }

  auto mask1 = "";
  auto mask2 = "0";
  auto mask3 = "1";
  auto mask4 = "0,1";

  auto driver_properties = lzt::get_driver_properties(driver);
  run_child_process(lzt::to_string(driver_properties.uuid), mask1,
                    num_total_devices);
  run_child_process(lzt::to_string(driver_properties.uuid), mask2,
                    lzt::get_ze_sub_device_count(devices[0]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask3,
                    lzt::get_ze_sub_device_count(devices[1]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask4,
                    lzt::get_ze_sub_device_count(devices[0]) +
                        lzt::get_ze_sub_device_count(devices[1]));
}

TEST(
    TestAffinity,
    GivenAffinityMaskSetWhenGettingSubDevicesThenCorrectNumberOfSubDevicesReturned) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (lzt::get_ze_sub_device_count(devices[0]) < 2) {
    LOG_WARNING << "Test not executed due to not enough devices/sub_devices";
    return;
  }

  auto mask1 = "0";
  auto mask2 = "0.0";
  auto mask3 = "0.1";
  auto mask4 = "0.1,0.0";

  auto driver_properties = lzt::get_driver_properties(driver);
  run_child_process(lzt::to_string(driver_properties.uuid), mask1,
                    lzt::get_ze_sub_device_count(devices[0]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask2, 1);
  run_child_process(lzt::to_string(driver_properties.uuid), mask3, 1);
  run_child_process(lzt::to_string(driver_properties.uuid), mask4, 2);
}

} // namespace