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

TEST(TestAffinity,
     GivenAffinityMaskSetWhenGettingDevicesThenCorrectNumberOfDevicesReturned) {

  uint32_t num_total_devices = 0;
  std::string affinity_mask = "";

  for (auto driver : lzt::get_all_driver_handles()) {

    auto devices = lzt::get_devices(driver);
    if (devices.empty()) {
      continue;
    }
    for (auto device : devices) {
      auto num_sub_devices = lzt::get_sub_device_count(device);
      num_total_devices += num_sub_devices ? num_sub_devices : 1;
    }

    auto num_masks = 1 << num_total_devices;

    for (uint32_t mask = 0; mask < num_masks; mask++) {
      std::stringstream os;
      os << std::hex << mask;
      affinity_mask = os.str();

      auto env = boost::this_process::environment();
      bp::environment child_env = env;

      child_env["ZE_AFFINITY_MASK"] = affinity_mask;

      auto driver_properties = lzt::get_driver_properties(driver);

      fs::path helper_path(boost::filesystem::current_path() / "device");
      std::vector<boost::filesystem::path> paths;
      paths.push_back(helper_path);
      fs::path helper = bp::search_path("test_affinity_helper", paths);
      bp::child get_devices_process(
          helper, lzt::to_string(driver_properties.uuid), child_env);

      get_devices_process.wait();

      uint32_t num_devices_mask = 0;
      auto temp = mask;
      while (temp) {
        num_devices_mask += temp & 0x1;
        temp >>= 1;
      }

      int num_devices_child = get_devices_process.exit_code();

      if (num_devices_child < 0) {
        FAIL() << "child process exited with error getting driver devices";
      }

      ASSERT_EQ(num_devices_child, num_devices_mask);
    }
  }
}

} // namespace