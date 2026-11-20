/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <chrono>
#include <thread>

#include "command_line/command_line.hpp"
#include "test_debug_common.hpp"

namespace lzt = level_zero_tests;

struct debug_options {
public:
  void parse_options(int argc, char **argv) {

    lzt::command_line_options options("Allowed Options");

    // Required Options:
    options.required(test_type_string, test_selected, "the test type to run");

    // Optional options:
    options.value(device_id_string, device_id_in,
                  "Device ID of device to test");
    options.value(module_string, module_name_in, "spv file to use");
    options.value(module_options_string, module_options_in,
                  "Build options for the module");
    options.flag(
        "no_ipc",
        "Disable process controls so tests can be run without parent process");
    options.flag(use_sub_devices_string,
                 "run the test on subdevices if available");
    options.value(index_string, index_in, "Index of this debuggee");
    options.flag(use_many_threads_string,
                 "use a larger number of threads than usual");

    options.parse(argc, argv, true);

    LOG_INFO << "[Application] TEST TYPE: " << test_selected;

    if (options.given(device_id_string)) {
      LOG_INFO << "[Application] Device ID option: " << device_id_in;
    }

    if (options.given(use_sub_devices_string)) {
      LOG_INFO << "[Application] Using sub devices";
      use_sub_devices = true;
    }

    if (options.given(module_string)) {
      use_custom_module = true;
      LOG_INFO << "[Application] Using Custom Module:  " << module_name_in;
    }

    if (options.given(module_options_string)) {
      LOG_INFO << "[Application] Using Custom Module Options:  "
               << module_options_in;
    }

    if (options.given("no_ipc")) {
      LOG_INFO << "[Application] Disabling IPC controls";
      enable_synchro = false;
    }

    if (options.given(index_string)) {
      LOG_INFO << "[Application] Setting index: " << index_in;
    }

    if (options.given(use_many_threads_string)) {
      use_many_threads = true;
      LOG_INFO << "[Application] Using many threads";
    }
  }

  bool use_sub_devices = false;
  bool use_many_threads = false;
  std::string device_id_in = "";
  std::string module_name_in = "";
  std::string module_options_in = "";
  std::string kernel_name_in = "";
  bool use_custom_module = false;
  debug_test_type_t test_selected = BASIC;
  bool enable_synchro = true;
  uint64_t index_in = 0;
};

#ifdef EXTENDED_TESTS
#include "test_debug_extended.hpp"
#endif
