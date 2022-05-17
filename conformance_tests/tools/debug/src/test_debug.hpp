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

namespace lzt = level_zero_tests;

typedef enum { DEBUG, INFO, WARNING } log_level_t;

bool check_event(zet_debug_session_handle_t &debug_session,
                 zet_debug_event_type_t eventType);
bool check_events(zet_debug_session_handle_t &debug_session,
                  std::vector<zet_debug_event_type_t> eventTypes);

void attach_and_get_module_event(uint32_t pid, process_synchro *synchro,
                                 ze_device_handle_t device,
                                 zet_debug_session_handle_t &debug_session,
                                 zet_debug_event_t &module_event);

void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                           const ze_device_thread_t &thread,
                           zet_debug_event_t &module_event, bool access_elf);

class ProcessLauncher {
public:
  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           std::string module_name, uint64_t index) {
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
                           (use_sub_devices ? "--use_sub_devices" : ""),
                           "--index=" + std::to_string(index),
                           bp::std_in < child_input);

    return debug_helper;
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           std::string module_name) {
    return launch_process(test_type, device, use_sub_devices, module_name, 0);
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices) {
    return launch_process(test_type, device, use_sub_devices, "", 0);
  }

protected:
  std::string bin_name = "test_debug_helper";
};

class zetDebugBaseSetup : public ProcessLauncher, public ::testing::Test {
protected:
  void SetUp() override { synchro = new process_synchro(true, true); }

  void TearDown() override {
    if (::testing::Test::HasFailure()) {
      LOG_WARNING << "[Debugger] Teardown with failure cleaning ";
      debugHelper.terminate();
      if ((lzt::sessionsAttachStatus.find(debugSession) !=
           lzt::sessionsAttachStatus.end()) &&
          (lzt::sessionsAttachStatus[debugSession])) {
        // Ingore detach result
        zetDebugDetach(debugSession);
      }
    }

    delete synchro;
  }

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

  bp::child debugHelper;
  zet_debug_session_handle_t debugSession;
};

#endif // TEST_DEBUG_HPP