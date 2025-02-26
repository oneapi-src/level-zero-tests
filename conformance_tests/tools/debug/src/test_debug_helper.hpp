/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_HELPER_HPP
#define TEST_DEBUG_HELPER_HPP

#include <chrono>
#include <thread>

#include <boost/program_options.hpp>

#include "test_debug_common.hpp"

namespace po = boost::program_options;
namespace lzt = level_zero_tests;

struct debug_options {
public:
  void parse_options(int argc, char **argv) {

    po::options_description desc("Allowed Options");
    auto options = desc.add_options();

    // Required Options:
    options(test_type_string,
            po::value<uint32_t>((uint32_t *)&test_selected)->required(),
            "the test type to run");

    // Optional options:
    options(device_id_string, po::value<std::string>(&device_id_in),
            "Device ID of device to test");
    options(module_string, po::value<std::string>(&module_name_in),
            "spv file to use");
    options(module_options_string, po::value<std::string>(&module_options_in),
            "Build options for the module");
    options(
        "no_ipc",
        "Disable process controls so tests can be run without parent process");
    options(use_sub_devices_string, "run the test on subdevices if available");
    options(index_string, po::value<uint64_t>(&index_in),
            "Index of this debuggee");
    options(use_many_threads_string,
            "use a larger number of threads than usual");

    std::vector<std::string> parser(argv + 1, argv + argc);
    po::parsed_options parsed_options = po::command_line_parser(parser)
                                            .options(desc)
                                            .allow_unregistered()
                                            .run();

    po::variables_map variables_map;
    po::store(parsed_options, variables_map);
    po::notify(variables_map);

    LOG_INFO << "[Application] TEST TYPE: " << test_selected;

    if (variables_map.count(device_id_string)) {
      LOG_INFO << "[Application] Device ID option: "
               << variables_map[device_id_string].as<std::string>() << " "
               << device_id_in;
    }

    if (variables_map.count(use_sub_devices_string)) {
      LOG_INFO << "[Application] Using sub devices";
      use_sub_devices = true;
    }

    if (variables_map.count(module_string)) {
      use_custom_module = true;
      LOG_INFO << "[Application] Using Custom Module:  " << module_name_in;
    }

    if (variables_map.count(module_options_string)) {
      LOG_INFO << "[Application] Using Custom Module Options:  "
               << module_options_in;
    }

    if (variables_map.count("no_ipc")) {
      LOG_INFO << "[Application] Disabling IPC controls";
      enable_synchro = false;
    }

    if (variables_map.count(index_string)) {
      index_in = variables_map[index_string].as<uint64_t>();
      LOG_INFO << "[Application] Setting index: " << index_in;
    }

    if (variables_map.count(use_many_threads_string)) {
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

#endif // TEST_DEBUG_HPP
