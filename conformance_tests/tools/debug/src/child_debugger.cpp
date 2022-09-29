/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <boost/program_options.hpp>

#include "test_debug.hpp"

namespace po = boost::program_options;

struct debugger_options {
public:
  void parse_options(int argc, char **argv) {

    po::options_description desc("Allowed Options");
    auto options = desc.add_options();

    std::string device_id_string = "device_id";
    std::string index_string = "index";
    // Required Options:
    options(device_id_string.c_str(),
            po::value<std::string>(&sub_device_id_in)->required(),
            "Device ID of device to test");
    options(index_string.c_str(), po::value<uint64_t>(&index_in)->required(),
            "Index of this debuggee");
    options(use_sub_devices_string, "Use subdevices");
    options(test_type_string,
            po::value<uint32_t>((uint32_t *)&test_selected)->required(),
            "the test type to run");

    std::vector<std::string> parser(argv + 1, argv + argc);
    po::parsed_options parsed_options = po::command_line_parser(parser)
                                            .options(desc)
                                            .allow_unregistered()
                                            .run();

    po::variables_map variables_map;
    po::store(parsed_options, variables_map);
    po::notify(variables_map);

    LOG_INFO << "[Child Debugger] sub device ID: "
             << variables_map[device_id_string].as<std::string>();

    if (variables_map.count(use_sub_devices_string)) {
      LOG_INFO << "[Child Debugger] Using sub devices";
      use_sub_devices = true;
    }
  }

  bool use_sub_devices = false;
  std::string sub_device_id_in;
  uint64_t index_in = 0;
  debug_test_type_t test_selected = BASIC;
};

int main(int argc, char **argv) {

  debugger_options options;
  options.parse_options(argc, argv);

  LOG_DEBUG << "[Child Debugger] INDEX:  " << options.index_in;
  auto index = options.index_in;
  process_synchro synchro(true, true, index);

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_ERROR << "[Child Debugger] zeInit failed";
    exit(1);
  }

  auto driver = lzt::get_default_driver();
  auto device = lzt::find_device(driver, options.sub_device_id_in.c_str(),
                                 options.use_sub_devices);

  if (device) {
    auto device_properties = lzt::get_device_properties(device);
    LOG_DEBUG << "[Child Debugger] Found device: " << options.sub_device_id_in
              << "  " << device_properties.name;
  } else {
    LOG_ERROR << "[Child Debugger] Could not find matching device";
    exit(1);
  }
  LOG_DEBUG << "[Child Debugger] Launching child application";

  ProcessLauncher launcher;
  auto debug_helper = launcher.launch_process(
      options.test_selected, device, options.use_sub_devices, "", index);

  // inform parent of my child's pid

  // we have the device, now create a debug session
  zet_debug_config_t debug_config = {};
  debug_config.pid = debug_helper.id();
  LOG_DEBUG << "[Child Debugger] Attaching to child application with PID: "
            << debug_helper.id();

  auto debugSession = lzt::debug_attach(device, debug_config);
  if (!debugSession) {
    LOG_ERROR << "[Child Debugger] Failed to attach to start a debug session";
    exit(1);
  }
  LOG_DEBUG << "[Child Debugger] Notifying child application";
  synchro.update_child_application_pid(debug_helper.id());
  if (options.test_selected != BASIC) {
    debug_helper.detach();
  }
  synchro.notify_application();
  lzt::debug_detach(debugSession);

  LOG_DEBUG << "[Child Debugger] Waiting for application to exit";
  debug_helper.wait();
  LOG_DEBUG << "[Child Debugger] Detaching";
  exit(debug_helper.exit_code());
}
