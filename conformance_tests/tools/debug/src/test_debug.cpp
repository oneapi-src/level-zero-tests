/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

namespace {

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenValidDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    auto device_properties = lzt::get_device_properties(device);
    auto properties = lzt::get_debug_properties(device);
    if (ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags == 0) {
      LOG_INFO << "Device " << device_properties.name
               << " does not support debug";
    } else {
      LOG_INFO << "Device " << device_properties.name << " supports debug";
    }
  }
}

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenSubDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto &device : lzt::get_devices(driver)) {
    for (auto &sub_device : lzt::get_ze_sub_devices(device)) {
      auto device_properties = lzt::get_device_properties(sub_device);
      auto properties = lzt::get_debug_properties(sub_device);
      if (ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags == 0) {
        LOG_INFO << "Device " << device_properties.name
                 << " does not support debug";
      } else {
        LOG_INFO << "Device " << device_properties.name << " supports debug";
      }
    }
  }
}

class zetDebugAttachDetachTest : public ::testing::Test {
protected:
  void run_test(std::vector<ze_device_handle_t> devices, bool use_sub_devices);
};

void zetDebugAttachDetachTest::run_test(std::vector<ze_device_handle_t> devices,
                                        bool use_sub_devices) {

  bi::shared_memory_object::remove("debug_bool");
  bi::shared_memory_object shm(bi::create_only, "debug_bool", bi::read_write);
  shm.truncate(sizeof(bool));
  bi::mapped_region region(shm, bi::read_write);
  *(static_cast<bool *>(region.get_address())) = false;

  bi::named_mutex::remove("debugger_mutex");
  bi::named_mutex mutex(bi::create_only, "debugger_mutex");

  bi::named_condition::remove("debug_bool_set");
  bi::named_condition condition(bi::create_only, "debug_bool_set");

  for (auto &device : devices) {
    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;
    bp::child debug_helper(helper, lzt::to_string(device_properties.uuid),
                           (use_sub_devices ? "1" : "0"),
                           bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);

    // notify debugged process that this process has attached
    mutex.lock();
    *(static_cast<bool *>(region.get_address())) = true;
    mutex.unlock();
    condition.notify_all();

    debug_helper.wait(); // we don't care about the child processes exit code at
                         // the moment
    lzt::debug_detach(debug_session);
  }
  bi::shared_memory_object::remove("debug_bool");
  bi::named_mutex::remove("debugger_mutex");
  bi::named_condition::remove("debug_bool_set");
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenSubDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  std::vector<ze_device_handle_t> all_sub_devices = {};
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                       sub_devices.end());
  }

  run_test(all_sub_devices, true);
}

} // namespace
