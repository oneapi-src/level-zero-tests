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

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
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
  options(device_id_string, po::value<std::string>(&device_id_in)->required(),
          "Device ID of device to test");

  // Optional options:
  options(module_string, po::value<std::string>(&module_name_in),
          "spv file to use");
  options(
      "no_ipc",
      "Disable process controls so tests can be run without parent process");
  options(use_sub_devices_string, "run the test on subdevices if available");

  std::vector<std::string> parser(argv + 1, argv + argc);
  po::parsed_options parsed_options =
      po::command_line_parser(parser).options(desc).allow_unregistered().run();

  po::variables_map variables_map;
  po::store(parsed_options, variables_map);
  po::notify(variables_map);

  LOG_INFO << "[Application] TEST TYPE: " << test_selected;
  LOG_INFO << "[Application] device ID: "
           << variables_map[device_id_string].as<std::string>() << " "
           << device_id_in;

  if (variables_map.count(use_sub_devices_string)) {
    LOG_INFO << "[Application] Using sub devices";
    use_sub_devices = true;
  }

  if (variables_map.count(module_string)) {
    use_custom_module = true;
    LOG_INFO << "[Application] Using Custom Module:  " << module_name_in;
  }
  if (variables_map.count("no_ipc")) {
    LOG_INFO << "[Application] Disabling IPC controls";
    enable_synchro = false;
  }
}

  bool use_sub_devices = false;
  std::string device_id_in = "";
  std::string module_name_in = "";
  bool use_custom_module = false;
  debug_test_type_t test_selected = BASIC;
  bool enable_synchro = true;
};



#ifdef EXTENDED_TESTS
#include "test_debug_extended.hpp"
#endif

#endif // TEST_DEBUG_HPP
