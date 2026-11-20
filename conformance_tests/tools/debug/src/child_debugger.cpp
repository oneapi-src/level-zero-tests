/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <thread>

#include "command_line/command_line.hpp"
#include "test_debug.hpp"
#include "test_debug_utils.hpp"

using lzt::to_u32;

struct debugger_options {
public:
  void parse_options(int argc, char **argv) {

    lzt::command_line_options options("Allowed Options");

    std::string device_id_string = "device_id";
    std::string index_string = "index";
    std::string pid_string = "app_pid";
    std::string verify_events_string = "verify_events";

    options.value(pid_string, app_pid_in, "Application PID");
    options.flag(verify_events_string, "Verify Events");
    options.flag(use_sub_devices_string, "Use subdevices");

    // Required Options:
    options.required(device_id_string, sub_device_id_in,
                     "Device ID of device to test");
    options.required(index_string, index_in, "Index of this debuggee");
    options.required(test_type_string, test_selected, "the test type to run");

    options.parse(argc, argv, true);

    LOG_INFO << "[Child Debugger] sub device ID: " << sub_device_id_in;

    if (options.given(use_sub_devices_string)) {
      LOG_INFO << "[Child Debugger] Using sub devices";
      use_sub_devices = true;
    }

    if (options.given(verify_events_string)) {
      LOG_INFO << "[Child Debugger] Verifying events from debuggee";
      verify_events = true;
    }

    if (options.given(pid_string)) {
      LOG_INFO << "[Child Debugger] Application PID: " << app_pid_in;
    }
  }

  bool use_sub_devices = false;
  bool verify_events = false;
  std::string sub_device_id_in;
  uint64_t index_in = 0;
  uint32_t app_pid_in = 0;
  debug_test_type_t test_selected = BASIC;
};

int main(int argc, char **argv) {

  debugger_options options;
  try {
    options.parse_options(argc, argv);
  } catch (const lzt::command_line_error &e) {
    LOG_ERROR << e.what();
    exit(1);
  }

  LOG_DEBUG << "[Child Debugger] INDEX:  " << options.index_in;
  auto index = options.index_in;
  process_synchro synchro(true, true, static_cast<int>(index));

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

  zet_debug_config_t debug_config = {};

  ProcessLauncher launcher;
  boost::asio::io_context debug_io_context;
  bp::process debug_helper{debug_io_context};

  if (options.app_pid_in) {
    // attach to pid from options
    LOG_DEBUG << "[Child Debugger] Received PID option: " << options.app_pid_in;
    debug_config.pid = options.app_pid_in;
  } else {
    LOG_DEBUG << "[Child Debugger] Launching child application";
    debug_helper =
        launcher.launch_process(options.test_selected, device,
                                options.use_sub_devices, "", index, false);
    debug_config.pid = to_u32(debug_helper.id());
  }

  // we have the device, now create a debug session
  LOG_DEBUG << "[Child Debugger] Attaching to child application with PID: "
            << debug_config.pid;

  auto debugSession = lzt::debug_attach(device, debug_config);
  if (!debugSession) {
    LOG_ERROR << "[Child Debugger] Failed to attach to start a debug session";
    exit(1);
  }

  if (options.app_pid_in) {
    LOG_DEBUG << "[Child Debugger] Notifying parent that debugger is attached";
    synchro.notify_parent();

    LOG_DEBUG << "[Child Debugger] Waiting for parent signal to proceed";
    synchro.wait_for_parent_signal();
    synchro.clear_parent_signal();
    LOG_DEBUG << "[Child Debugger] Received parent signal, proceeding";

    if (options.verify_events) {
      LOG_DEBUG << "[Child Debugger] Verifying Events";
      process_synchro synchro0(true, false, 0);

      std::vector<zet_debug_event_type_t> expectedEvents = {
          ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD};

      if (!check_events(debugSession, expectedEvents)) {
        LOG_DEBUG << "[Child Debugger] Did not receive expected events";
        exit(1);
      }

      ze_device_thread_t device_thread;
      device_thread.slice = UINT32_MAX;
      device_thread.subslice = UINT32_MAX;
      device_thread.eu = UINT32_MAX;
      device_thread.thread = UINT32_MAX;

      synchro0.wait_for_application_signal();
      LOG_INFO << "[Child Debugger] Sleeping to wait for device threads";
      std::this_thread::sleep_for(std::chrono::seconds(6));

      LOG_INFO << "[Child Debugger] Sending interrupt";
      lzt::debug_interrupt(debugSession, device_thread);

      std::vector<ze_device_thread_t> stopped_threads;
      if (!find_stopped_threads(debugSession, device, device_thread, true,
                                stopped_threads)) {

        LOG_INFO << "[Child Debugger] Did not find stopped threads";
        exit(1);
      }

      LOG_DEBUG << "[Child Debugger] Writing to memory";
      zet_debug_memory_space_desc_t memory_space_desc = {};
      uint64_t gpu_buffer_va = 0;
      if (!synchro0.get_app_gpu_buffer_address(gpu_buffer_va)) {
        LOG_DEBUG << "[Child Debugger] Could not get a valid GPU buffer VA";
        exit(1);
      }
      synchro0.clear_application_signal();
      memory_space_desc.address = gpu_buffer_va;
      memory_space_desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
      memory_space_desc.stype = ZET_STRUCTURE_TYPE_DEBUG_MEMORY_SPACE_DESC;

      uint8_t *buffer = new uint8_t[1];
      buffer[0] = 0;
      LOG_INFO << "[Child Debugger] Writing to address: " << std::hex
               << gpu_buffer_va;
      lzt::debug_write_memory(debugSession, device_thread, memory_space_desc, 1,
                              buffer);
      delete[] buffer;
      print_thread("Resuming device thread ", device_thread, DEBUG);
      lzt::debug_resume(debugSession, device_thread);
    }
    LOG_DEBUG << "[Child Debugger] Detaching and exiting";

    lzt::debug_detach(debugSession);

    exit(0);

  } else {
    LOG_DEBUG << "[Child Debugger] Notifying child application";
    synchro.notify_application();

    LOG_DEBUG << "[Child Debugger] Detaching";
    lzt::debug_detach(debugSession);
    LOG_DEBUG << "[Child Debugger] Waiting for application to exit";
    debug_helper.wait();
    exit(debug_helper.exit_code());
  }
}
