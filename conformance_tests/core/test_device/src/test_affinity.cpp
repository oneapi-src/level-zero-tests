/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <regex>
#include <algorithm>
#include <chrono>
#include <future>

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

std::string get_result(bp::ipstream &stream) {

  std::string result;
  std::getline(stream, result);

  return result;
}

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

  LOG_INFO << "[Affinity Mask: " << affinity_mask << "]";

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
    EXPECT_EQ(num_devices_child, num_devices_mask);
  }
}

void print_devices(ze_device_handle_t device, int level) {
  if (lzt::get_ze_sub_device_count(device)) {
    for (auto &subdevice : lzt::get_ze_sub_devices(device)) {
      print_devices(subdevice, level + 1);
    }
    std::cout << " Parent Device - lvl: " << level << "\n";
  } else {
    std::cout << level << "\n";
  }
}

uint16_t get_device_count(ze_device_handle_t device) {
  if (lzt::get_ze_sub_device_count(device)) {
    int sum = 1; // 1 for this device
    for (auto &subdevice : lzt::get_ze_sub_devices(device)) {
      sum += get_device_count(subdevice);
    }
    return sum;
  } else {
    return 1;
  }
}

uint16_t get_leaf_device_count(ze_device_handle_t device) {
  if (lzt::get_ze_sub_device_count(device)) {
    int sum = 0;
    for (auto &subdevice : lzt::get_ze_sub_devices(device)) {
      sum += get_leaf_device_count(subdevice);
    }
    return sum;
  } else {
    return 1;
  }
}

std::string get_affinity_mask_string(ze_device_handle_t device,
                                     uint32_t &input_mask,
                                     uint16_t parent_index, uint16_t my_index,
                                     uint16_t &devices_present) {

  std::stringstream output_mask;
  auto device_count = get_device_count(device);
  if (device_count == 1) {
    if (input_mask & 0x01) { // device present in input_mask
      // tell my parent that I am present
      devices_present++;
      // add this device to output mask
      output_mask << my_index;
    }
    input_mask >>= 1;
  } else { // device has subdevices
    std::stringstream temp_output_mask;
    auto sub_device_index = 0;
    devices_present = 0;
    auto subdevices = lzt::get_ze_sub_devices(device);
    for (auto &sub_device : subdevices) {
      auto result = get_affinity_mask_string(sub_device, input_mask, my_index,
                                             sub_device_index, devices_present);
      if (!result.empty()) {
        temp_output_mask << result + ",";
      }
      sub_device_index++;
    }
    devices_present += 1;

    // we have determined wether our descendant devices are present
    // if all our descendants are present, then just ouput our device index
    // **if only some of our devices are present, then concatenate those
    // strings**
    // if none of our descendants are present, then do nothing
    if (devices_present == device_count) {
      output_mask << my_index;
    } else if (devices_present == 1) {
    } else {

      auto temp_string = temp_output_mask.str();
      if (temp_string.size() > 1) {
        temp_string.erase(temp_string.end() - 1);
      }
      auto prefix = std::to_string(my_index) + ".";
      if (!temp_string.empty()) {
        temp_string.insert(0, prefix);
      }
      auto pos = temp_string.find(",");
      while (pos != std::string::npos) {
        temp_string.insert(pos + 1, prefix);
        pos = temp_string.find(",", pos + 1);
      }

      output_mask << temp_string;
    }
  }
  return output_mask.str();
}

TEST(
    TestAffinity,
    GivenAffinityMaskSetWhenGettingDevicesThenCorrectNumberOfDevicesReturnedInAllCases) {

  for (auto driver : lzt::get_all_driver_handles()) {
    auto devices = lzt::get_devices(driver);
    uint32_t num_leaf_devices = 0;
    if (devices.empty()) {
      continue;
    }
    for (auto device : devices) {
      num_leaf_devices += get_leaf_device_count(device);
    }

    // in each mask, either a device is present or it is not,
    // so for N devices there are 2^N possible masks
    auto num_masks = 1 << num_leaf_devices;
    for (uint32_t mask = 0; mask < num_masks; mask++) {
      uint32_t num_devices_mask = 0;
      auto temp = mask;
      while (temp) {
        num_devices_mask += temp & 0x1;
        temp >>= 1;
      }
      // an unset mask means all devices should be returned
      if (num_devices_mask == 0) {
        num_devices_mask = num_leaf_devices;
      }

      // build this affinity mask string
      auto temp_mask = mask;
      std::string affinity_mask_string;
      uint16_t parent_index = 0;
      for (int root_device = 0; root_device < devices.size(); root_device++) {
        uint16_t device_present = 0;

        auto temp_string =
            get_affinity_mask_string(devices[root_device], temp_mask,
                                     parent_index, root_device, device_present);
        if (!temp_string.empty()) {
          if (!affinity_mask_string.empty()) {
            affinity_mask_string += ",";
          }
          affinity_mask_string += temp_string;
        }
        parent_index++;
      }

      // launch child process with constructed mask
      auto driver_properties = lzt::get_driver_properties(driver);
      // most significant 32-bits of UUID are timestamp, which can change, so
      // zero-out least significant 32-bits are driver version
      memset(&driver_properties.uuid.id[4], 0, 4);

      run_child_process(lzt::to_string(driver_properties.uuid),
                        affinity_mask_string, num_devices_mask);
    }
  }
}

TEST(
    TestAffinity,
    GivenSpecificAffinityMaskSetWhenGettingDevicesThenCorrectNumberOfDevicesReturned) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (devices.size() < 2) {
    LOG_WARNING << "Test not executed due to not enough devices";
    return;
  }

  uint32_t num_leaf_devices = 0;
  for (int i = 0; i < devices.size(); i++) {
    num_leaf_devices += get_leaf_device_count(devices[i]);
  }

  auto mask1 = "";
  auto mask2 = "0";
  auto mask3 = "1";
  auto mask4 = "0,1";

  auto driver_properties = lzt::get_driver_properties(driver);
  memset(&driver_properties.uuid.id[4], 0, 4);
  run_child_process(lzt::to_string(driver_properties.uuid), mask1,
                    num_leaf_devices);
  run_child_process(lzt::to_string(driver_properties.uuid), mask2,
                    get_leaf_device_count(devices[0]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask3,
                    get_leaf_device_count(devices[1]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask4,
                    get_leaf_device_count(devices[0]) +
                        get_leaf_device_count(devices[1]));
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

  auto subdevices = lzt::get_ze_sub_devices(devices[0]);
  auto mask1 = "0";
  auto mask2 = "0.0";
  auto mask3 = "0.1";
  auto mask4 = "0.1,0.0";

  auto driver_properties = lzt::get_driver_properties(driver);
  memset(&driver_properties.uuid.id[4], 0, 4);
  run_child_process(lzt::to_string(driver_properties.uuid), mask1,
                    get_leaf_device_count(devices[0]));

  run_child_process(lzt::to_string(driver_properties.uuid), mask2,
                    get_leaf_device_count(subdevices[0]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask3,
                    get_leaf_device_count(subdevices[1]));
  run_child_process(lzt::to_string(driver_properties.uuid), mask4,
                    get_leaf_device_count(subdevices[0]) +
                        get_leaf_device_count(subdevices[1]));

  //
}

} // namespace
