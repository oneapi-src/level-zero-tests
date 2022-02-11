/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_debug_common.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

const uint16_t eventsTimeoutMS = 30000;
const uint16_t eventsTimeoutS = 30;
const uint16_t timeoutThreshold = 4;

namespace lzt = level_zero_tests;

typedef enum { SINGLE_THREAD, GROUP_OF_THREADS, ALL_THREADS } num_threads_t;

#define CLEAN_AND_ASSERT(condition, debug_session, helper)                     \
  do {                                                                         \
    if (!condition) {                                                          \
      lzt::debug_detach(debug_session);                                        \
      helper.terminate();                                                      \
      ASSERT_TRUE(false);                                                      \
    }                                                                          \
  } while (0)

class zetDebugBaseSetup : public ::testing::Test {
protected:
  void SetUp() override { synchro = new process_synchro(true, true); }

  void TearDown() override { delete synchro; }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           std::string module_name) {
    auto device_properties = lzt::get_device_properties(device);
    std::string device_id = lzt::to_string(device_properties.uuid);
    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path(bin_name, paths);
    bp::opstream child_input;
    std::string module_name_option = "";
    if (!module_name.empty())
      module_name_option = "--module=" + module_name;

    bp::child debug_helper(helper, "--test_type=" + std::to_string(test_type),
                           "--device_id=" + device_id, module_name_option,
                           (use_sub_devices ? "--use_sub_devices=1" : ""),
                           bp::std_in < child_input);

    return debug_helper;
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices) {
    return launch_process(test_type, device, use_sub_devices, "");
  }

  std::string bin_name = "test_debug_helper";
  process_synchro *synchro;

public:
  static bool is_debug_supported(ze_device_handle_t device) {
    auto device_properties = lzt::get_device_properties(device);
    auto properties = lzt::get_debug_properties(device);

    if (ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags) {
      LOG_INFO << "[Debugger] Device " << device_properties.name
               << " has debug support";
      return true;
    } else {
      LOG_WARNING << "[Debugger] Device " << device_properties.name
                  << " does not support debug";
      return false;
    }
  }
};

#endif // TEST_DEBUG_HPP