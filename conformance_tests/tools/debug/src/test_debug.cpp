/*
 *
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_debug.hpp"
#include "test_debug_utils.hpp"

namespace lzt = level_zero_tests;

#ifdef WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include <mutex>
#include <thread>

namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenValidDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    zetDebugBaseSetup::is_debug_supported(device);
  }
}

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenSubDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto &device : lzt::get_devices(driver)) {
    for (auto &sub_device : lzt::get_ze_sub_devices(device)) {
      zetDebugBaseSetup::is_debug_supported(sub_device);
    }
  }
}

void zetDebugAttachDetachTest::run_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices,
    bool reattach) {
  const uint64_t timeoutMs = 100;

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(BASIC, device, use_sub_devices);
    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();

    if (!reattach) {
      debugSession = lzt::debug_attach(device, debug_config);
      if (!debugSession) {
        FAIL() << "[Debugger] Failed to attach to start a debug session";
      }
      synchro->notify_application();
      LOG_INFO << "[Debugger] Detaching";
      lzt::debug_detach(debugSession);
      debugHelper.wait();
      EXPECT_EQ(debugHelper.exit_code(), 0);
    } else {
      int loop = 1;
      for (loop = 1; loop < 11; loop++) {
        LOG_INFO << "[Debugger] Attaching. Loop " << loop;
        auto debugSession = lzt::debug_attach(device, debug_config);
        if (!debugSession) {
          FAIL()
              << "[Debugger] Failed to attach to start a debug session. Loop "
              << loop;
        }

        // delay last detach to happen after application finishes
        if (loop < 10) {
          LOG_INFO << "[Debugger] Detaching. Loop " << loop;
          lzt::debug_detach(debugSession);
        } else {
          zet_debug_event_t event = {};
          auto readEventResult =
              lzt::debug_read_event(debugSession, event, timeoutMs, true);
          if (readEventResult == ZE_RESULT_SUCCESS &&
              (event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK)) {
            LOG_INFO << "[Debugger] Event with NEED_ACK flag";
            lzt::debug_ack_event(debugSession, &event);
            LOG_INFO << "[Debugger] Event acknowledged";
          } else {
            LOG_INFO << "[Debugger] No event to read";
          }

          synchro->notify_application();
          std::this_thread::sleep_for(std::chrono::seconds(1));
          debugHelper.terminate();
          LOG_INFO << "[Debugger] LAST Detach after aplication finished. Loop "
                   << loop;
          lzt::debug_detach(debugSession);
        }
      }
    }
  }
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, false);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenSubDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_test(all_sub_devices, true, false);
}

TEST_F(zetDebugAttachDetachTest,
       GivenPreviousDebugSessionDetachedWhenAttachingThenReAttachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_test(devices, false, true);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenSubDevicePreviousDebugSessionDetachedWhenAttachingThenReAttachIsSuccessful) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_test(all_sub_devices, true, true);
}

bp::child launch_child_debugger_process(debug_test_type_t test_type,
                                        std::string device_id,
                                        bool use_sub_devices, uint32_t index,
                                        uint32_t app_pid = 0,
                                        bool verify_events = false) {

  fs::path debugger_path(fs::current_path() / "debug");
  std::vector<fs::path> paths;
  paths.push_back(debugger_path);
  fs::path helper = bp::search_path("child_debugger", paths);

  bp::child debugger(helper, "--test_type=" + std::to_string(test_type),
                     "--device_id=" + device_id,
                     (use_sub_devices ? "--use_sub_devices" : " "),
                     "--index=" + std::to_string(index),
                     app_pid ? "--app_pid=" + std::to_string(app_pid) : " ",
                     verify_events ? "--verify_events" : " ");

  return debugger;
}

void zetDebugAttachDetachTest::run_multidevice_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {

  synchro->clear_debugger_signal();
  auto device0 = devices[0];
  auto device1 = devices[1];
  auto device_properties = lzt::get_device_properties(device0);
  LOG_DEBUG << "[Debugger] Launching application on " << device_properties.uuid
            << " " << device_properties.name;
  debugHelper = launch_process(BASIC, devices[0], use_sub_devices);
  zet_debug_config_t debug_config = {};
  debug_config.pid = debugHelper.id();
  LOG_DEBUG << "[Debugger] Launched application with PID:  " << debugHelper.id()
            << " using " << device_properties.uuid << " "
            << device_properties.name;

  debugSession = lzt::debug_attach(device0, debug_config);
  if (!debugSession) {
    FAIL() << "[Debugger] Failed to attach to start a debug session";
  }

  LOG_DEBUG << "[Debugger] Notifying applications of attach";
  synchro->notify_application();

  // create new process
  auto device_properties_1 = lzt::get_device_properties(device1);
  LOG_DEBUG << "[Debugger] Launching new debugger on "
            << device_properties_1.uuid << " " << device_properties_1.name;
  auto debugger = launch_child_debugger_process(
      BASIC, lzt::to_string(device_properties_1.uuid), use_sub_devices, 1);

  LOG_DEBUG << "[Debugger] Launched new debugger with PID:  " << debugger.id()
            << " using " << device_properties_1.uuid << " "
            << device_properties_1.name;

  lzt::debug_detach(debugSession);
  debugHelper.wait();
  LOG_INFO << "[Debugger] Main debugger detaching";

  LOG_INFO << "[Debugger] Waiting for child debugger to exit";
  debugger.wait();
  LOG_INFO << "[Debugger] Child debugger exited, finishing test";

  ASSERT_EQ(debugHelper.exit_code(), 0);
  ASSERT_EQ(debugger.exit_code(), 0);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenApplicationsExecutingOnDifferentSubdevicesWhenAttachingThenDifferentDebuggersCanAttachAndDetachSuccessfully) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  bool test_run = false;
  for (auto &device : devices) {
    print_device(device);

    auto device_properties = lzt::get_device_properties(device);
    auto sub_devices = lzt::get_ze_sub_devices(device);
    auto initial_count = sub_devices.size();
    sub_devices.erase(std::remove_if(sub_devices.begin(), sub_devices.end(),
                                     [](ze_device_handle_t &device) {
                                       return (!is_debug_supported(device));
                                     }),
                      sub_devices.end());
    auto final_count = sub_devices.size();
    if (final_count < 2) {
      LOG_WARNING << "Skipping device with < 2  "
                  << ((final_count < initial_count) ? "debug capable " : "")
                  << "subdevices: " << device_properties.uuid << " "
                  << device_properties.name;
      continue;
    } else {
      test_run = true;
      run_multidevice_test(sub_devices, true);
    }
  }

  if (!test_run) {
    GTEST_SKIP() << "No device with multiple supported subdevices available, "
                    "test not run";
  }
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenApplicationsExecutingOnDifferentDevicesInMultiDeviceSystemWhenAttachingThenDifferentDebuggersCanAttachAndDetachSuccessfully) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  auto initial_count = devices.size();
  devices.erase(std::remove_if(devices.begin(), devices.end(),
                               [](ze_device_handle_t &device) {
                                 return (!is_debug_supported(device));
                               }),
                devices.end());
  auto final_count = devices.size();

  if (final_count < 2) {
    LOG_WARNING << "Not enough "
                << ((final_count < initial_count) ? "debug capable " : "")
                << "devices to run multi-device test for driver";
  } else {
    run_multidevice_test(devices, false);
  }
}

void zetDebugAttachDetachTest::
    run_attach_detach_to_multiple_applications_on_different_devs_test(
        std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {

  auto device0 = devices[0];
  auto device1 = devices[1];
  LOG_DEBUG << "[Debugger] Test attaching/detaching to 2 applications";

  auto device_properties_0 = lzt::get_device_properties(device0);
  auto device_properties_1 = lzt::get_device_properties(device1);

  LOG_DEBUG << "[Debugger] Launching application 1 on "
            << device_properties_0.uuid << " " << device_properties_0.name;
  auto debug_helper_0 = launch_process(BASIC, device0, use_sub_devices);
  LOG_DEBUG << "[Debugger] Launching application 2 on "
            << device_properties_1.uuid << " " << device_properties_1.name;
  auto debug_helper_1 = launch_process(BASIC, device1, use_sub_devices);

  zet_debug_config_t debug_config_0 = {};
  debug_config_0.pid = debug_helper_0.id();

  zet_debug_config_t debug_config_1 = {};
  debug_config_1.pid = debug_helper_1.id();

  LOG_DEBUG << "[Debugger] Attaching to application 1 at pid="
            << debug_config_0.pid;
  auto debug_session_0 = lzt::debug_attach(device0, debug_config_0);

  LOG_DEBUG << "[Debugger] Attaching to application 2 at pid="
            << debug_config_1.pid;
  auto debug_session_1 = lzt::debug_attach(device1, debug_config_1);

  if (!debug_session_0 || !debug_session_1) {
    debug_helper_0.terminate();
    debug_helper_1.terminate();
    FAIL() << "[Debugger] Failed to attach to 1 or both applications start a "
              "debug session";
  }
  synchro->notify_application(); // both applications will see debugger signal

  LOG_DEBUG << "[Debugger] Waiting for application process entry debug events";
  zet_debug_event_t event;
  lzt::debug_read_event(debug_session_0, event, eventsTimeoutMS, false);
  ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY);
  lzt::debug_read_event(debug_session_1, event, eventsTimeoutMS, false);
  ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY);

  LOG_DEBUG << "[Debugger] Waiting for application module load debug events";
  for (int i = 0; i < 5;
       i++) { // BASIC test loads debug_add which has 5 kernels
    lzt::debug_read_event(debug_session_0, event, eventsTimeoutMS, false);
    ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD);
    lzt::debug_ack_event(debug_session_0, &event);
    lzt::debug_read_event(debug_session_1, event, eventsTimeoutMS, false);
    ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD);
    lzt::debug_ack_event(debug_session_1, &event);
  }

  LOG_DEBUG << "[Debugger] Waiting for application module unload debug events";
  for (int i = 0; i < 5; i++) {
    lzt::debug_read_event(debug_session_0, event, eventsTimeoutMS, false);
    ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD);
    lzt::debug_read_event(debug_session_1, event, eventsTimeoutMS, false);
    ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD);
  }

  LOG_DEBUG << "[Debugger] Waiting for application process exit debug events";
  lzt::debug_read_event(debug_session_0, event, eventsTimeoutMS, false);
  ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT);
  lzt::debug_read_event(debug_session_1, event, eventsTimeoutMS, false);
  ASSERT_EQ(event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT);

  LOG_DEBUG << "[Debugger] Waiting for application 1 to finish";
  debug_helper_0.wait();
  LOG_DEBUG << "[Debugger] Waiting for application 2 to finish";
  debug_helper_1.wait();

  LOG_INFO << "[Debugger] Detaching from application 1";
  lzt::debug_detach(debug_session_0);
  LOG_INFO << "[Debugger] Detaching from application 2";
  lzt::debug_detach(debug_session_1);

  synchro->clear_application_signal();

  ASSERT_EQ(debug_helper_0.exit_code(), 0);
  ASSERT_EQ(debug_helper_1.exit_code(), 0);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDeviceSupportsDebugAttachWhenAttachingToMultipleApplicationsThenAttachAndDetachIsSuccessful) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  auto initial_count = devices.size();
  devices.erase(std::remove_if(devices.begin(), devices.end(),
                               [](ze_device_handle_t &device) {
                                 return (!is_debug_supported(device));
                               }),
                devices.end());
  auto final_count = devices.size();

  if (final_count < 2) {
    LOG_WARNING << "Not enough "
                << ((final_count < initial_count) ? "debug capable " : "")
                << "devices to run multi-device test for driver";
  } else {
    run_attach_detach_to_multiple_applications_on_different_devs_test(devices,
                                                                      false);
  }
}

void zetDebugAttachDetachTest::run_use_same_device_test(
    const zet_device_handle_t &device) {

  auto device_properties = lzt::get_device_properties(device);

  LOG_DEBUG << "[Debugger] Launching application on " << device_properties.uuid
            << " " << device_properties.name;

  debugHelper = launch_process(BASIC, device, false);

  auto debugger_1 = launch_child_debugger_process(
      BASIC, lzt::to_string(device_properties.uuid), false, 1);

  zet_debug_config_t debug_config = {};
  debug_config.pid = debugHelper.id();

  // Second debugger waits until first has run and exited
  LOG_INFO << "[Debugger] Waiting for first debugger to finish";
  debugger_1.wait();
  EXPECT_EQ(debugger_1.exit_code(), 0);

  LOG_INFO << "[Debugger] First debugger finished, second attaching";
  debugSession = lzt::debug_attach(device, debug_config);
  synchro->notify_application();

  LOG_INFO << "[Debugger] Detaching";
  lzt::debug_detach(debugSession);
  LOG_INFO << "[Debugger] Waiting for application to finish";
  debugHelper.wait();
  EXPECT_EQ(debugHelper.exit_code(), 0);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDifferentApplicationsUsingSameDeviceWhenBeingDebuggedThenAttachAndDetachIsSuccessful) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {
    print_device(device);
    synchro->clear_debugger_signal();
    if (!is_debug_supported(device)) {
      LOG_WARNING << "Device does not support debug";
      continue;
    }

    run_use_same_device_test(device);
  }
}

void zetDebugAttachDetachTest::run_new_debugger_attach_test(
    const std::vector<ze_device_handle_t> &devices, bool verify_events) {

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    auto device_properties = lzt::get_device_properties(device);
    synchro->clear_debugger_signal();

    // launch application, using synchro 0
    debugHelper =
        launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device, false);
    LOG_DEBUG << "[Debugger] Launched application process with PID:  "
              << debugHelper.id() << " using " << device_properties.uuid << " "
              << device_properties.name;

    // launch debugger 1 using synchro 1
    auto debugger = launch_child_debugger_process(
        LONG_RUNNING_KERNEL_INTERRUPTED, lzt::to_string(device_properties.uuid),
        false, 1, debugHelper.id(), verify_events);
    LOG_DEBUG << "[Debugger] Launched debugger with PID:  " << debugger.id()
              << " using " << device_properties.uuid << " "
              << device_properties.name;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    process_synchro synchro_1(true, false, 1);

    // wait until first debugger attaches to application
    synchro_1.wait_for_child_signal();
    synchro_1.clear_child_signal();

    // first debugger has attached, terminate without it detaching
    debugger.terminate();
    LOG_INFO << "[Debugger] Terminated child debugger without detaching\n\n";

    // launch debugger 2 using synchro 1
    auto debugger_2 = launch_child_debugger_process(
        LONG_RUNNING_KERNEL_INTERRUPTED, lzt::to_string(device_properties.uuid),
        false, 1, debugHelper.id(), verify_events);
    LOG_DEBUG << "[Debugger] Launched new debugger with PID:  "
              << debugger_2.id() << " using " << device_properties.uuid << " "
              << device_properties.name;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    process_synchro synchro_2 = process_synchro(true, false, 1);

    LOG_INFO << "[Parent] Waiting until 2nd debugger has attached";
    synchro_2.wait_for_child_signal();
    synchro_2.clear_child_signal();
    // notify the application that debugger 2 has attached using synchro 0
    LOG_INFO
        << "[Parent] Notifying application that 2nd debugger has attached ";
    synchro->notify_application();

    // notify debugger 2 that we have notified application
    LOG_INFO
        << "[Parent] Notifying 2nd Debugger that application is proceeding";
    synchro_2.notify_child();
    LOG_INFO << "Waiting on 2nd debugger to finish";
    debugger_2.wait();
    EXPECT_EQ(debugger_2.exit_code(), 0);

    if (!verify_events) {
      debugHelper.terminate();
    } else {
      LOG_INFO << "Waiting on application to finish";
      debugHelper.wait();
      EXPECT_EQ(debugHelper.exit_code(), 0);
    }
  }
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenExistingDebuggerExitsWithoutDetachingWhenDebuggingApplicationThenNewDebuggerCanAttach) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_new_debugger_attach_test(devices, false);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenExistingDebuggerExitsWithoutDetachingWhenDebuggingApplicationThenNewDebuggerCanVerifyEvents) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_new_debugger_attach_test(devices, true);
}

void zetDebugEventReadTest::run_test(std::vector<ze_device_handle_t> &devices,
                                     bool use_sub_devices,
                                     debug_test_type_t test_type) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(test_type, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();
    debugSession = lzt::debug_attach(device, debug_config);
    if (!debugSession) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_application();
    LOG_INFO << "[Debugger] Listening for events";

    uint16_t eventNum = 0;
    uint16_t moduleLoadCount = 0;
    uint16_t moduleUnloadCount = 0;
    bool gotProcEntry = false;
    bool gotProcExit = false;

    zet_debug_event_t debug_event;
    do {
      ze_result_t result = lzt::debug_read_event(debugSession, debug_event,
                                                 eventsTimeoutMS, false);
      if (ZE_RESULT_SUCCESS != result) {
        break;
      }
      LOG_INFO << "[Debugger] received event: "
               << lzt::debuggerEventTypeString[debug_event.type];
      eventNum++;

      if (ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY == debug_event.type) {
        EXPECT_EQ(eventNum, 1);
        gotProcEntry = true;
      } else if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        moduleLoadCount++;
        EXPECT_TRUE(debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK);
        lzt::debug_ack_event(debugSession, &debug_event);
      } else if (ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        moduleUnloadCount++;
      } else if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        gotProcExit = true;
        break;
      }

    } while (ZET_DEBUG_EVENT_TYPE_INVALID != debug_event.type);

    EXPECT_GE(moduleLoadCount, 1);
    EXPECT_EQ(moduleLoadCount, moduleUnloadCount);
    EXPECT_TRUE(gotProcEntry);
    EXPECT_TRUE(gotProcExit);

    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

void zetDebugEventReadTest::run_attach_after_module_created_destroyed_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices,
    debug_test_type_t test_type) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(test_type, device, use_sub_devices);
    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();

    synchro->wait_for_application_signal();

    debugSession = lzt::debug_attach(device, debug_config);
    if (!debugSession) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }
    synchro->clear_application_signal();
    synchro->notify_application();

    std::vector<zet_debug_event_type_t> expectedEvents = {};
    if (test_type == ATTACH_AFTER_MODULE_CREATED) {
      expectedEvents = {
          ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY,
          ZET_DEBUG_EVENT_TYPE_MODULE_LOAD,
      };

      if (!check_events_unordered(debugSession, expectedEvents)) {
        FAIL() << "[Debugger] Did not receive expected events";
      }

    } else {
      expectedEvents = {
          ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY,
          ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT,
      };

      if (!check_events(debugSession, expectedEvents)) {
        FAIL() << "[Debugger] Did not receive expected events";
      }
    }

    lzt::debug_detach(debugSession);
    debugHelper.wait();
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

void zetDebugEventReadTest::run_proc_entry_exit_test(
    std::vector<zet_device_handle_t> &devices, bool use_sub_devices,
    debug_test_type_t test_type) {

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    std::map<int, int> ordinalCQs;
    int totalNumCQs = get_numCQs_per_ordinal(device, ordinalCQs);
    synchro->clear_debugger_signal();
    debugHelper = launch_process(test_type, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();
    debugSession = lzt::debug_attach(device, debug_config);
    if (!debugSession) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    LOG_INFO << "[Debugger] Listening for events";

    zet_debug_event_t debug_event;
    ze_result_t result;
    for (int i = 0; i < 3; i++) {
      LOG_DEBUG << "[Debugger]--- Loop " << i << " ---";

      // Expect timeout with no event since no CQ is created.
      // No event expected after attaching
      result = lzt::debug_read_event(debugSession, debug_event, 2, true);
      EXPECT_EQ(result, ZE_RESULT_NOT_READY);

      // check CQs creation
      for (int queueNum = 1; queueNum <= totalNumCQs; queueNum++) {

        // let the app create a CQ
        synchro->notify_application();
        synchro->wait_for_application_signal();
        synchro->clear_application_signal();

        // only the first queue creation should send the event
        if (queueNum == 1) {
          if (!check_event(debugSession, ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY)) {
            FAIL() << "[Debugger] Did not recieve "
                      "ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY";
          }
        } else {
          result = lzt::debug_read_event(debugSession, debug_event, 2, true);
          EXPECT_EQ(result, ZE_RESULT_NOT_READY);
        }
      }

      // let the app to continue after creating the last CQ.
      synchro->notify_application();

      // check CQs destruction
      for (int queueNum = 1; queueNum <= totalNumCQs; queueNum++) {

        synchro->wait_for_application_signal();
        synchro->clear_application_signal();

        // only the last CQ destruction should send the event
        if (queueNum == totalNumCQs) {
          if (!check_event(debugSession, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT)) {
            FAIL() << "[Debugger] Did not recieve "
                      "ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT";
          }
        } else {
          result = lzt::debug_read_event(debugSession, debug_event, 2, true);
          EXPECT_EQ(result, ZE_RESULT_NOT_READY);
        }

        // let the app destroy another CQ
        synchro->notify_application();
      }

    } // loops

    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

void zetDebugEventReadTest::run_detach_no_ack_module_create_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(BASIC, device, use_sub_devices);
    zet_debug_event_t module_event;
    attach_and_get_module_event(debugHelper.id(), synchro, device, debugSession,
                                module_event);

    lzt::debug_detach(debugSession);
    debugHelper.wait();
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, BASIC);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableSubDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_test(all_sub_devices, true, BASIC);
}

TEST_F(zetDebugEventReadTest,
       GivenDeviceWhenCreatingMultipleModulesThenMultipleEventsReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, MULTIPLE_MODULES_CREATED);
}

TEST_F(zetDebugEventReadTest,
       GivenSubDeviceWhenCreatingMultipleModulesThenMultipleEventsReceived) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_test(all_sub_devices, true, MULTIPLE_MODULES_CREATED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableDeviceWhenAttachingAfterModuleCreatedThenEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_attach_after_module_created_destroyed_test(devices, false,
                                                 ATTACH_AFTER_MODULE_CREATED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableSubDeviceWhenAttachingAfterModuleCreatedThenEventReceived) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_attach_after_module_created_destroyed_test(all_sub_devices, true,
                                                 ATTACH_AFTER_MODULE_CREATED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugEnabledDeviceWhenAttachingAfterCreatingAndDestroyingModuleThenNoModuleEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_attach_after_module_created_destroyed_test(devices, false,
                                                 ATTACH_AFTER_MODULE_DESTROYED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugEnabledSubDeviceWhenAttachingAfterCreatingAndDestroyingModuleThenNoModuleEventReceived) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_attach_after_module_created_destroyed_test(all_sub_devices, true,
                                                 ATTACH_AFTER_MODULE_DESTROYED);
}

TEST_F(zetDebugEventReadTest,
       GivenDebuggerAttachedProcessEntryAndExitAreSentForCQs) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_proc_entry_exit_test(devices, false, MULTIPLE_CQ);
}

TEST_F(zetDebugEventReadTest,
       GivenDebuggerAttachedtoSubDeviceProcessEntryAndExitAreSentForCQs) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_proc_entry_exit_test(all_sub_devices, true, MULTIPLE_CQ);
}

TEST_F(zetDebugEventReadTest,
       GivenDebuggerAttachedProcessEntryAndExitAreSentForImmediateCMDLists) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_proc_entry_exit_test(devices, false, MULTIPLE_IMM_CL);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebuggerAttachedtoSubDeviceProcessEntryAndExitAreSentForImmediateCMDLists) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_proc_entry_exit_test(all_sub_devices, true, MULTIPLE_IMM_CL);
}

void verify_single_application_host_thread(
    const zet_debug_session_handle_t &debug_session) {

  zet_debug_event_t debug_event;
  LOG_DEBUG << "[Debugger] Waiting for application process entry debug event "
               "due to second thread starting";
  lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
  ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY);

  LOG_INFO << "[Debugger] Waiting for application module load debug events";
  for (int i = 0; i < 5; i++) {
    lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
    ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD);
    lzt::debug_ack_event(debug_session, &debug_event);
  }
  LOG_INFO << "[Debugger] Waiting for application module load debug events";
  for (int i = 0; i < 5; i++) {
    lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
    ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD);
  }
  LOG_INFO << "[Debugger] Waiting for application process exit debug event";
  lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
  ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT);
}

void read_and_verify_events_multithreaded_app(
    const zet_debug_session_handle_t &debug_session) {

  zet_debug_event_t debug_event;
  lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
  LOG_INFO << "[Debugger] received event: "
           << lzt::debuggerEventTypeString[debug_event.type];
  ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY);

  auto num_application_host_threads_to_verify = 2;
  auto broken = false;
  for (int i = 0; i < 20; i++) {
    // module load and unload events can be interleaved if threads run
    // concurrently if threads run sequentially we will expect a process exit
    // event in this loop
    lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
    LOG_INFO << "[Debugger] received event: "
             << lzt::debuggerEventTypeString[debug_event.type];

    ASSERT_TRUE((ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) ||
                (ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD == debug_event.type) ||
                (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type));
    if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
      lzt::debug_ack_event(debug_session, &debug_event);
    }

    if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
      // This means the first thread exited before the second started
      // we should break this loop
      num_application_host_threads_to_verify--;
      if (num_application_host_threads_to_verify == 1) {
        broken = true;
        break;
      }
    }
  }

  if (broken) {
    // we should expect a process entry/load/unload/exit sequence
    verify_single_application_host_thread(debug_session);
  } else {
    // we should expect a process exit event
    LOG_INFO << "[Debugger] Waiting for application process exit debug event";
    lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
    LOG_INFO << "[Debugger] received event: "
             << lzt::debuggerEventTypeString[debug_event.type];
    ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT);
  }
}

void zetDebugEventReadTest::run_multithreaded_application_test(
    std::vector<zet_device_handle_t> &devices, bool use_sub_devices) {

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(MULTIPLE_THREADS, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();
    debugSession = lzt::debug_attach(device, debug_config);
    if (!debugSession) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_application();

    // BASIC test loads debug_add which has 5 kernels
    read_and_verify_events_multithreaded_app(debugSession);

    LOG_INFO << "[Debugger] Waiting for application to finish";
    debugHelper.wait();

    LOG_INFO << "[Debugger] Detaching from application";
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebuggerAttachedWhenRunningMultithreadedApplicationThenCorrectEventsReadAndAcknowledgementsSucceed) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_multithreaded_application_test(devices, false);
}

std::mutex module_load_mutex;
std::condition_variable module_load_cv;
bool module_loaded = false;
void read_and_verify_events_debugger_thread(
    const zet_debug_session_handle_t &debug_session, uint64_t *gpu_buffer_va) {

  LOG_INFO << "[Debugger] Event Read Thread starting...";
  zet_debug_event_t debug_event;

  std::vector<zet_debug_event_type_t> expectedEvents = {
      ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD};

  if (!check_events(debug_session, expectedEvents)) {
    FAIL() << "[Debugger] Did not receive expected events";
  }

  {
    // we have received module load event and acknowledged it
    std::lock_guard<std::mutex> lk(module_load_mutex);
    module_loaded = true;
    module_load_cv.notify_one();
  }

  lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS, false);
  LOG_INFO << "[Debugger] received event: "
           << lzt::debuggerEventTypeString[debug_event.type];
  ASSERT_EQ(debug_event.type, ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED);

  // write to kernel buffer to signal to application to end
  zet_debug_memory_space_desc_t memory_space_desc = {};

  memory_space_desc.address = *gpu_buffer_va;
  memory_space_desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
  memory_space_desc.stype = ZET_STRUCTURE_TYPE_DEBUG_MEMORY_SPACE_DESC;

  uint8_t *buffer = new uint8_t[1];
  buffer[0] = 0;
  auto thread = debug_event.info.thread.thread;
  LOG_INFO << "[Debugger] Writing to address: " << std::hex << *gpu_buffer_va;
  lzt::debug_write_memory(debug_session, thread, memory_space_desc, 1, buffer);
  delete[] buffer;
  print_thread("Resuming device thread ", thread, DEBUG);
  lzt::debug_resume(debug_session, thread);

  LOG_INFO << "[Debugger] Waiting for module unload and process exit events";

  expectedEvents = {ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD,
                    ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT};
  if (!check_events(debug_session, expectedEvents)) {
    FAIL() << "[Debugger] Did not receive expected events";
  }
}

void zetDebugEventReadTest::run_read_events_in_separate_thread_test(
    std::vector<zet_device_handle_t> &devices, bool use_sub_devices) {

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device,
                                 use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debugHelper.id();
    debugSession = lzt::debug_attach(device, debug_config);
    if (!debugSession) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_application();

    uint64_t gpu_buffer_va = 0;
    std::thread event_read_thread(read_and_verify_events_debugger_thread,
                                  debugSession, &gpu_buffer_va);
    synchro->wait_for_application_signal();
    if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va)) {
      FAIL() << "[Debugger] Could not get a valid GPU buffer VA";
    }
    synchro->clear_application_signal();

    ze_device_thread_t device_thread;
    device_thread.slice = UINT32_MAX;
    device_thread.subslice = UINT32_MAX;
    device_thread.eu = UINT32_MAX;
    device_thread.thread = UINT32_MAX;

    LOG_INFO << "[Debugger] Main thread waiting for module load event";
    std::unique_lock<std::mutex> lk(module_load_mutex);
    module_load_cv.wait(lk, [] { return module_loaded; });
    lk.unlock();
    LOG_INFO << "[Debugger] Main thread sleeping to wait for device threads";
    std::this_thread::sleep_for(std::chrono::seconds(6));

    LOG_INFO << "[Debugger] Sending interrupt from main thread";
    lzt::debug_interrupt(debugSession, device_thread);

    LOG_INFO << "[Debugger] Waiting for application to finish";
    debugHelper.wait();

    event_read_thread.join();

    LOG_INFO << "[Debugger] Detaching from application";
    lzt::debug_detach(debugSession);

    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebuggerReadsEventsInDifferentThreadWhenRunningApplicationThenCorrectEventsReadAndAcknowledgementsSucceed) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_read_events_in_separate_thread_test(devices, false);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugDetachCalledWhenModuleCreateEventIsNotAckedThenKernelCompletesSuccessfully) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_detach_no_ack_module_create_test(devices, false);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugDetachCalledOnSubDeviceWhenModuleCreateEventIsNotAckedThenKernelCompletesSuccessfully) {
  auto all_sub_devices = lzt::get_all_sub_devices();

  run_detach_no_ack_module_create_test(all_sub_devices, true);
}

void zetDebugMemAccessTest::run_module_isa_elf_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(BASIC, device, use_sub_devices);
    zet_debug_event_t module_event;
    attach_and_get_module_event(debugHelper.id(), synchro, device, debugSession,
                                module_event);

    // ALL threads
    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;
    readWriteModuleMemory(debugSession, thread, module_event, false);
    lzt::debug_ack_event(debugSession, &module_event);
    lzt::debug_detach(debugSession);
    debugHelper.terminate();
  }
}

TEST_F(zetDebugMemAccessTest,
       GivenDebuggerAttachedAndModuleLoadedAccessISAAndELFMemory) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_module_isa_elf_test(devices, false);
}

TEST_F(zetDebugMemAccessTest,
       GivenDebuggerAttachedSubDeviceAndModuleLoadedAccessISAAndELFMemory) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_module_isa_elf_test(all_sub_devices, true);
}

TEST_F(
    zetDebugMemAccessTest,
    GivenDebuggerAttachedAndModuleLoadedWhenThreadStoppedThenDebuggerCanReadWriteToAllocatedBuffersInTheContextOfThread) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_read_write_module_and_memory_test(devices, false, 0, false);
}

TEST_F(
    zetDebugMemAccessTest,
    GivenDebuggerAttachedSubDeviceAndModuleLoadedWhenThreadStoppedThenDebuggerCanReadWriteToAllocatedBuffersInTheContextOfThread) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_read_write_module_and_memory_test(all_sub_devices, false, 0, true);
}

void run_register_set_properties_test(std::vector<ze_device_handle_t> devices) {
  for (auto &device : devices) {
    print_device(device);

    auto properties = lzt::get_register_set_properties(device);
    EXPECT_FALSE(properties.empty());
    zet_debug_regset_properties_t empty_properties = {};
    for (auto &property : properties) {
      EXPECT_NE(memcmp(&property, &empty_properties,
                       sizeof(zet_debug_regset_properties_t)),
                0);
    }
  }
}

TEST(zetDebugRegisterSetTest,
     GivenDeviceWhenGettingRegisterSetPropertiesThenValidPropertiesReturned) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_register_set_properties_test(devices);
}

TEST(
    zetDebugRegisterSetTest,
    GivenSubDeviceWhenGettingRegisterSetPropertiesThenValidPropertiesReturned) {
  run_register_set_properties_test(lzt::get_all_sub_devices());
}

void zetDebugReadWriteRegistersTest::run_read_write_registers_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device))
      continue;

    synchro->clear_debugger_signal();
    debugHelper = launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device,
                                 use_sub_devices);

    zet_debug_event_t module_event;
    attach_and_get_module_event(debugHelper.id(), synchro, device, debugSession,
                                module_event);

    if (module_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
      LOG_DEBUG << "[Debugger] Acking event: "
                << lzt::debuggerEventTypeString[module_event.type];
      lzt::debug_ack_event(debugSession, &module_event);
    }

    uint64_t gpu_buffer_va = 0;
    synchro->wait_for_application_signal();
    if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va)) {
      FAIL() << "[Debugger] Could not get a valid GPU buffer VA";
    }
    synchro->clear_application_signal();

    zet_debug_memory_space_desc_t memorySpaceDesc;
    memorySpaceDesc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
    int sizeToRead = 512;
    uint8_t *kernel_buffer = new uint8_t[sizeToRead];
    // set buffer[0] to 0 to break the loop. See debug_loop.cl
    kernel_buffer[0] = 0;
    memorySpaceDesc.address = gpu_buffer_va;

    ze_device_thread_t device_threads = {};
    device_threads.slice = UINT32_MAX;
    device_threads.subslice = UINT32_MAX;
    device_threads.eu = UINT32_MAX;
    device_threads.thread = UINT32_MAX;

    LOG_INFO << "[Debugger] Stopping all device threads";
    // give time to app to launch the kernel
    std::this_thread::sleep_for(std::chrono::seconds(6));

    lzt::debug_interrupt(debugSession, device_threads);

    std::vector<ze_device_thread_t> stopped_threads;
    if (!find_stopped_threads(debugSession, device, device_threads, true,
                              stopped_threads)) {
      FAIL() << "[Debugger] Did not find stopped threads";
    }

    auto register_set_properties = lzt::get_register_set_properties(device);
    LOG_INFO << "[Debugger] Reading/Writing registers on interrupted threads";
    for (auto &stopped_thread : stopped_threads) {
      int regSetNumber = 1;

      for (auto &register_set : register_set_properties) {
        auto buffer_size = register_set.byteSize * register_set.count;
        void *buffer = lzt::allocate_host_memory(register_set.byteSize *
                                                 register_set.count);
        void *buffer_copy = lzt::allocate_host_memory(register_set.byteSize *
                                                      register_set.count);

        uint32_t regSetType = register_set.type;
        std::memset(buffer, 0xaa, buffer_size);

        // Will NOT test write-only registers since cannot restore oringal
        // content
        if (register_set.generalFlags & ZET_DEBUG_REGSET_FLAG_READABLE) {
          LOG_DEBUG << "[Debugger] Register set " << regSetNumber << " type "
                    << regSetType << " is readable";

          // read all registers in this register set
          lzt::debug_read_registers(debugSession, stopped_thread,
                                    register_set.type, 0, register_set.count,
                                    buffer);

          // save the contents for write test
          std::memcpy(buffer_copy, buffer, buffer_size);

          if (register_set.generalFlags & ZET_DEBUG_REGSET_FLAG_WRITEABLE) {
            LOG_DEBUG << "[Debugger] Register set " << regSetNumber << " type "
                      << regSetType << " is writeable";
            std::memset(buffer, 0xaa, buffer_size);

            lzt::debug_write_registers(debugSession, stopped_thread,
                                       register_set.type, 0, register_set.count,
                                       buffer);

            lzt::debug_read_registers(debugSession, stopped_thread,
                                      register_set.type, 0, register_set.count,
                                      buffer);

            for (int i = 0; i < buffer_size; i++) {
              if (static_cast<char>(0xaa) != static_cast<char *>(buffer)[i]) {
                FAIL() << "[Debugger] register set " << regSetNumber
                       << " FAILED write test. Expected "
                       << static_cast<char>(0xaa) << " , found "
                       << static_cast<char *>(buffer)[i];
              }
            }

            LOG_INFO << "[Debugger] Validating register set " << regSetNumber
                     << " type " << regSetType << " written successfully";

            // write back the original contents
            lzt::debug_write_registers(debugSession, stopped_thread,
                                       register_set.type, 0, register_set.count,
                                       buffer_copy);
          } else {
            LOG_INFO << "[Debugger] Register set " << regSetNumber << " type "
                     << regSetType << " is NOT writeable";
          }
        } else {
          LOG_INFO << "[Debugger] Register set " << regSetNumber << " type "
                   << regSetType << " si NOT readable";
        }

        lzt::free_memory(buffer);
        lzt::free_memory(buffer_copy);
        regSetNumber++;
      }
    }

    lzt::debug_write_memory(debugSession, device_threads, memorySpaceDesc, 1,
                            kernel_buffer);
    delete[] kernel_buffer;

    LOG_INFO << "[Debugger] resuming interrupted threads";
    lzt::debug_resume(debugSession, device_threads);
    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

TEST_F(
    zetDebugReadWriteRegistersTest,
    GivenActiveDebugSessionWhenReadingAndWritingRegistersThenValidDataReadAndDataWrittenSuccessfully) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_read_write_registers_test(devices, false);
}

TEST_F(
    zetDebugReadWriteRegistersTest,
    GivenActiveDebugSessionWhenReadingAndWritingSubDeviceRegistersThenValidDataReadAndDataWrittenSuccessfully) {
  auto all_sub_devices = lzt::get_all_sub_devices();
  run_read_write_registers_test(all_sub_devices, true);
}

void zetDebugThreadControlTest::SetUpThreadControl(ze_device_handle_t &device,
                                                   bool use_sub_devices) {

  LOG_INFO << "[Debugger] Setting up for thread control tests ";
  synchro->clear_debugger_signal();
  debugHelper =
      launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device, use_sub_devices);

  zet_debug_event_t module_event;
  attach_and_get_module_event(debugHelper.id(), synchro, device, debugSession,
                              module_event);

  ASSERT_TRUE(module_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK);
  LOG_DEBUG << "[Debugger] Acking event: "
            << lzt::debuggerEventTypeString[module_event.type];
  lzt::debug_ack_event(debugSession, &module_event);

  uint64_t gpu_buffer_va = 0;
  synchro->wait_for_application_signal();
  if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va)) {
    FAIL() << "[Debugger] Could not get a valid GPU buffer VA";
  }
  synchro->clear_application_signal();
  memorySpaceDesc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
  memorySpaceDesc.address = gpu_buffer_va;

  // stop all threads
  ze_device_thread_t thread;
  thread.slice = UINT32_MAX;
  thread.subslice = UINT32_MAX;
  thread.eu = UINT32_MAX;
  thread.thread = UINT32_MAX;

  LOG_INFO << "[Debugger] Interrupting all threads";
  // give time to app to launch the kernel
  std::this_thread::sleep_for(std::chrono::seconds(6));

  lzt::debug_interrupt(debugSession, thread);
  stopped_threads = {};
  if (!find_stopped_threads(debugSession, device, thread, true,
                            stopped_threads)) {
    FAIL() << "[Debugger] Did not find stopped threads";
  }

  // sort to avoid ordering differences
  std::sort(stopped_threads.begin(), stopped_threads.end(),
            smaller_thread_functor());
}

void zetDebugThreadControlTest::run_alternate_stop_resume_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device)) {
      continue;
    }

    std::vector<ze_device_thread_t> stoppedThreadsCheck;
    std::vector<ze_device_thread_t> threadsToCheck;

    SetUpThreadControl(device, use_sub_devices);
    if (::testing::Test::HasFailure()) {
      FAIL() << "[Debugger] Failed to setup Thread Control tests";
    }

    // iterate over threads and resume
    LOG_INFO << "[Debugger] ######### Ressuming Odd threads ##########";
    int i = 1;
    for (auto &stopped_thread : stopped_threads) {

      if (i % 2) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    LOG_INFO
        << "[Debugger] ######### Interrupting Odd AND resumming Even threads "
           "##########";
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (i % 2) {
        print_thread("[Debugger] Interrupting thread ", stopped_thread, INFO);
        lzt::debug_interrupt(debugSession, stopped_thread);

        // Confirm the thread was effectively stopped
        if (!find_stopped_threads(debugSession, device, stopped_thread, true,
                                  stoppedThreadsCheck)) {
          FAIL() << "[Debugger] Did not find stopped threads";
        }
        if (!is_thread_in_vector(stopped_thread, stoppedThreadsCheck)) {
          FAIL() << "[Debugger] Did not interrupt requested thread";
        }
      }

      if (!(i % 2)) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    LOG_INFO
        << "[Debugger] ######### Interrupting Even threads AND resumming Odd "
           "threads ##########";
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (!(i % 2)) {
        print_thread("[Debugger] Interrupting thread ", stopped_thread, INFO);
        lzt::debug_interrupt(debugSession, stopped_thread);

        // Confirm the thread was effectively stopped
        if (!find_stopped_threads(debugSession, device, stopped_thread, true,
                                  stoppedThreadsCheck)) {
          FAIL() << "[Debugger] Did not find stopped threads";
        }

        if (!is_thread_in_vector(stopped_thread, stoppedThreadsCheck)) {
          FAIL() << "[Debugger] Did not interrupt requested thread";
        }
      }
      if (i % 2) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));

    LOG_INFO << "[Debugger] ######### Interrupting Odd threads ##########";
    threadsToCheck.clear();
    i = 1;
    for (auto &stopped_thread : stopped_threads) {

      if (i % 2) {
        print_thread("[Debugger] Interrupting thread ", stopped_thread, INFO);
        lzt::debug_interrupt(debugSession, stopped_thread);
        threadsToCheck.push_back(stopped_thread);
      }
      i++;
    }

    // check for odd threads events and threads stopped all together
    if (!find_multi_event_stopped_threads(debugSession, device, threadsToCheck,
                                          true, stoppedThreadsCheck)) {
      FAIL() << "[Debugger] Did not interrupt Odd threads";
    }

    LOG_INFO
        << "[Debugger] ######### Checking ALL threads are stopped ##########";
    stoppedThreadsCheck = get_stopped_threads(debugSession, device);
    EXPECT_EQ(stoppedThreadsCheck.size(), stopped_threads.size());

    EXPECT_EQ(debugHelper.running(), true);

    LOG_INFO << "[Debugger] ######### Resuming Even threads ######";
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (!(i % 2)) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_EQ(debugHelper.running(), true);

    LOG_INFO << "[Debugger] ######### Ressuming Odd threads ##########";
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (i % 2) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }

    LOG_INFO << "[Debugger] ######### Checking ALL threads are running ######";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stoppedThreadsCheck.clear();
    stoppedThreadsCheck = get_stopped_threads(debugSession, device);
    EXPECT_EQ(stoppedThreadsCheck.size(), 0);

    LOG_INFO << "[Debugger] ######### Interrupting Even threads ##########";
    threadsToCheck.clear();
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (!(i % 2)) {
        print_thread("[Debugger] Interrupting thread ", stopped_thread, INFO);
        lzt::debug_interrupt(debugSession, stopped_thread);
        threadsToCheck.push_back(stopped_thread);
      }
      i++;
    }

    // check for Even threads events and threads stopped all together
    if (!find_multi_event_stopped_threads(debugSession, device, threadsToCheck,
                                          true, stoppedThreadsCheck)) {
      FAIL() << "[Debugger] Did not interrupt Even threads";
    }

    LOG_INFO << "[Debugger] ######### Resuming Even threads ##########";
    i = 1;
    for (auto &stopped_thread : stopped_threads) {
      if (!(i % 2)) {
        print_thread("[Debugger] Resuming thread ", stopped_thread, INFO);
        lzt::debug_resume(debugSession, stopped_thread);
      }
      i++;
    }

    LOG_INFO
        << "[Debugger] ######### Checking ALL threads are running ##########";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stoppedThreadsCheck.clear();
    stoppedThreadsCheck = get_stopped_threads(debugSession, device);
    EXPECT_EQ(stoppedThreadsCheck.size(), 0);

    EXPECT_EQ(debugHelper.running(), true);

    // stop all threads
    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;

    LOG_INFO << "[Debugger] ######### Interrupting all threads ##########";
    lzt::debug_interrupt(debugSession, thread);
    if (!find_stopped_threads(debugSession, device, thread, true,
                              stoppedThreadsCheck)) {
      FAIL() << "[Debugger] Did not find stopped threads";
    }
    // All threads should be stopped
    EXPECT_EQ(stoppedThreadsCheck.size(), stopped_threads.size());

    LOG_DEBUG << "[Debugger] Writing to GPU buffer to break kernel loop ";
    const int bufferSize = 1;
    uint8_t *buffer = new uint8_t[bufferSize];
    buffer[0] = 0;
    lzt::debug_write_memory(debugSession, thread, memorySpaceDesc, 1, buffer);
    LOG_INFO << "[Debugger] ######### Resuming all threads  ######### ";
    lzt::debug_resume(debugSession, thread);
    // Do not check for running threads because some will terminate the loop

    delete[] buffer;

    // verify helper completes
    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

void zetDebugThreadControlTest::run_interrupt_resume_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {

  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device)) {
      continue;
    }

    std::vector<ze_device_thread_t> newly_stopped_threads;
    std::vector<ze_device_thread_t> hierarchyThreads;
    ze_device_thread_t thread;
    bool breakKernelLoop = false;

    SetUpThreadControl(device, use_sub_devices);
    if (::testing::Test::HasFailure()) {
      FAIL() << "[Debugger] Failed to setup Thread Control tests";
    }

    // Resume all threads
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;

    LOG_DEBUG << "[Debugger] Resumming ALL threads";
    lzt::debug_resume(debugSession, thread);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    newly_stopped_threads.clear();
    newly_stopped_threads = get_stopped_threads(debugSession, device);
    // All threads should be running
    EXPECT_EQ(newly_stopped_threads.size(), 0);

    for (auto testType : numThreadsToString) {
      LOG_INFO
          << "[Debugger] ---- Testing interrupting and resuming threads on "
          << numThreadsToString[testType.first] << " ---- ";

      switch (testType.first) {
      case SINGLE_THREAD:

        thread = stopped_threads[0];
        print_thread("[Debugger] Interrupting thread: ", thread, INFO);

        break;
      case THREADS_IN_EU:

        thread.slice = UINT32_MAX;
        thread.subslice = UINT32_MAX;
        thread.eu = stopped_threads[0].eu;
        thread.thread = UINT32_MAX;
        print_thread("[Debugger] Interrupting EU threads: ", thread, INFO);
        hierarchyThreads =
            get_threads_in_eu(stopped_threads[0].eu, stopped_threads);

        break;
      case THREADS_IN_SUBSLICE:

        thread.slice = UINT32_MAX;
        thread.subslice = stopped_threads[0].subslice;
        thread.eu = UINT32_MAX;
        thread.thread = UINT32_MAX;
        print_thread("[Debugger] Interrupting Subslice threads: ", thread,
                     INFO);
        hierarchyThreads = get_threads_in_subSlice(stopped_threads[0].subslice,
                                                   stopped_threads);

        break;
      case THREADS_IN_SLICE:

        thread.slice = stopped_threads[0].slice;
        thread.subslice = UINT32_MAX;
        thread.eu = UINT32_MAX;
        thread.thread = UINT32_MAX;
        print_thread("[Debugger] Interrupting Slice threads: ", thread, INFO);
        hierarchyThreads =
            get_threads_in_slice(stopped_threads[0].slice, stopped_threads);

        break;
      case ALL_THREADS:

        thread.slice = UINT32_MAX;
        thread.subslice = UINT32_MAX;
        thread.eu = UINT32_MAX;
        thread.thread = UINT32_MAX;
        print_thread("[Debugger] Interrupting All threads: ", thread, INFO);

        break;
      default:
        FAIL() << "[Debugger] Invalid thread control test";
        break;
      }

      lzt::debug_interrupt(debugSession, thread);
      if (!find_stopped_threads(debugSession, device, thread, true,
                                newly_stopped_threads)) {
        FAIL() << "[Debugger] Did not find stopped threads";
      }

      // sort to avoid ordering differences
      std::sort(newly_stopped_threads.begin(), newly_stopped_threads.end(),
                smaller_thread_functor());

      switch (testType.first) {
      case SINGLE_THREAD:
        EXPECT_EQ(newly_stopped_threads.size(), 1);
        EXPECT_TRUE(are_threads_equal(thread, newly_stopped_threads[0]));
        break;
      case THREADS_IN_EU:
      case THREADS_IN_SUBSLICE:
      case THREADS_IN_SLICE:
        std::sort(hierarchyThreads.begin(), hierarchyThreads.end(),
                  smaller_thread_functor());
        EXPECT_EQ(newly_stopped_threads.size(), hierarchyThreads.size());
        EXPECT_TRUE(newly_stopped_threads == hierarchyThreads);
        break;
      case ALL_THREADS:
        EXPECT_EQ(newly_stopped_threads.size(), stopped_threads.size());
        EXPECT_TRUE(newly_stopped_threads == stopped_threads);
        breakKernelLoop = true;
        break;
      default:
        break;
      }

      if (breakKernelLoop) {
        LOG_DEBUG << "[Debugger] Writting to GPU buffer to break kernel loop ";
        const int bufferSize = 1;
        uint8_t *buffer = new uint8_t[bufferSize];
        memset(buffer, 0, bufferSize);
        lzt::debug_write_memory(debugSession, thread, memorySpaceDesc, 1,
                                buffer);
        delete[] buffer;
      }

      print_thread("[Debugger] Resuming thread: ", thread, INFO);
      lzt::debug_resume(debugSession, thread);
      std::this_thread::sleep_for(std::chrono::seconds(2));
      newly_stopped_threads.clear();
      newly_stopped_threads = get_stopped_threads(debugSession, device);
      // All threads should be running or terminated
      EXPECT_EQ(newly_stopped_threads.size(), 0);
    }

    // verify helper completes
    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

void zetDebugThreadControlTest::run_unavailable_thread_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device)) {
      continue;
    }

    ze_device_thread_t thread, threadToStop;
    std::vector<ze_device_thread_t> newly_stopped_threads;
    bool foundThread;

    SetUpThreadControl(device, use_sub_devices);
    if (::testing::Test::HasFailure()) {
      FAIL() << "[Debugger] Failed to setup Thread Control tests";
    }

    ze_device_properties_t deviceProperties =
        lzt::get_device_properties(device);
    LOG_INFO << "[Debugger] Device slices: " << deviceProperties.numSlices
             << " Sublices per slice: " << deviceProperties.numSubslicesPerSlice
             << " EU per SS:" << deviceProperties.numEUsPerSubslice
             << " Threads per EU:" << deviceProperties.numThreadsPerEU;

    // Find a thread that is not running. Start with the highest
    threadToStop.slice = deviceProperties.numSlices - 1;
    threadToStop.subslice = deviceProperties.numSubslicesPerSlice - 1;
    threadToStop.eu = deviceProperties.numEUsPerSubslice - 1;
    threadToStop.thread = deviceProperties.numThreadsPerEU - 1;

    foundThread = false;
    while (!foundThread) {

      if (!is_thread_in_vector(threadToStop, stopped_threads)) {
        foundThread = true;
        break;
      } else {
        if (threadToStop.thread == 0) {
          threadToStop.thread = deviceProperties.numThreadsPerEU;
          threadToStop.eu--;
        }
        if (threadToStop.eu == 0) {
          // give up after trying all EUs
          break;
        }

        threadToStop.thread--;
      }
    }

    // Interrupt already stopped thread
    print_thread(
        "[Debugger] Attempting to interrupt thread (already stopped): ",
        stopped_threads[0], INFO);
    EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE,
              zetDebugInterrupt(debugSession, stopped_threads[0]));

    // Resume all threads
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;

    LOG_DEBUG << "[Debugger] Resumming ALL threads";
    lzt::debug_resume(debugSession, thread);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    newly_stopped_threads.clear();
    newly_stopped_threads = get_stopped_threads(debugSession, device);
    // All threads should be running
    EXPECT_EQ(newly_stopped_threads.size(), 0);

    if (foundThread) {
      print_thread("[Debugger] Attempting to interrupt thread (not running): ",
                   threadToStop, INFO);
      lzt::debug_interrupt(debugSession, threadToStop);

      if (!check_event(debugSession, ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE)) {
        FAIL() << "[Debugger] Did not recieve THREAD_UNAVAILABLE event";
      }
    }

    LOG_INFO << "[Debugger] Interrupting all threads ";
    lzt::debug_interrupt(debugSession, thread);
    newly_stopped_threads.clear();
    if (!find_stopped_threads(debugSession, device, thread, true,
                              newly_stopped_threads)) {
      FAIL() << "[Debugger] Did not find stopped threads";
    }
    EXPECT_EQ(newly_stopped_threads.size(), stopped_threads.size());

    LOG_DEBUG << "[Debugger] Writting to GPU buffer to break kernel loop ";
    const int bufferSize = 1;
    uint8_t *buffer = new uint8_t[bufferSize];
    buffer[0] = 0;
    lzt::debug_write_memory(debugSession, thread, memorySpaceDesc, 1, buffer);
    LOG_INFO << "[Debugger] Resuming all threads ";
    lzt::debug_resume(debugSession, thread);
    delete[] buffer;

    // Allow enought time for all threads to terminate
    std::this_thread::sleep_for(std::chrono::seconds(10));
    newly_stopped_threads.clear();
    newly_stopped_threads = get_stopped_threads(debugSession, device);
    // All threads should be terminated
    EXPECT_EQ(newly_stopped_threads.size(), 0);

    print_thread(
        "[Debugger] Attempting to interrupt thread (with no threads running): ",
        threadToStop, INFO);
    lzt::debug_interrupt(debugSession, threadToStop);

    std::vector<zet_debug_event_type_t> expectedEvents = {
        ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT,
        ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE};

    if (!check_events(debugSession, expectedEvents)) {
      FAIL() << "[Debugger] Did not receive expected events";
    }

    // verify helper completes
    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}

TEST_F(
    zetDebugThreadControlTest,
    GivenAlternatingInterruptingAndResumingThreadsWhenDebuggingThenKernelCompletesSuccessfully) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_alternate_stop_resume_test(devices, false);
}

TEST_F(
    zetDebugThreadControlTest,
    GivenAlternatingInterruptingAndResumingThreadsOnSubDevicesWhenDebuggingThenKernelCompletesSuccessfully) {

  auto devices = lzt::get_all_sub_devices();
  run_alternate_stop_resume_test(devices, true);
}

TEST_F(
    zetDebugThreadControlTest,
    GivengInterruptingAndResumingThreadWhenDebuggingThenKernelCompletesSuccessfully) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_interrupt_resume_test(devices, false);
}

TEST_F(
    zetDebugThreadControlTest,
    GivengInterruptingAndResumingThreadWhenDebuggingOnSubDevicesThenKernelCompletesSuccessfully) {

  auto devices = lzt::get_all_sub_devices();
  run_interrupt_resume_test(devices, true);
}

TEST_F(zetDebugThreadControlTest,
       GivengInterruptingThreadNotRunningThenUnavaiableEventIsReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_unavailable_thread_test(devices, false);
}

TEST_F(
    zetDebugThreadControlTest,
    GivengInterruptingThreadNotRunningOnSubDeviceThenUnavailableEventIsReceived) {

  auto devices = lzt::get_all_sub_devices();
  run_unavailable_thread_test(devices, true);
}

void MultiDeviceDebugTest::run_multidevice_single_application_test(
    std::vector<ze_device_handle_t> &devices, bool use_sub_devices) {

  lzt::sort_devices(devices);
  auto device0 = devices[0];
  auto device1 = devices[1];

  auto device_properties = lzt::get_device_properties(device0);
  auto device_properties_1 = lzt::get_device_properties(device1);

  LOG_DEBUG << "[Debugger] Launching application using two devices:";
  LOG_DEBUG << "[Debugger] Device 0: " << device_properties.name << " "
            << device_properties.deviceId << " " << device_properties.uuid;
  LOG_DEBUG << "[Debugger] Device 1: " << device_properties_1.name << " "
            << device_properties_1.deviceId << " " << device_properties_1.uuid;

  debugHelper = launch_process(USE_TWO_DEVICES, nullptr, use_sub_devices);
  zet_debug_config_t debug_config = {};
  debug_config.pid = debugHelper.id();
  LOG_DEBUG << "[Debugger] Launched application with PID:  "
            << debugHelper.id();

  auto debug_session_0 = lzt::debug_attach(device0, debug_config);
  if (!debug_session_0) {
    FAIL() << "[Debugger] Failed to attach to start a debug session";
  }

  auto debug_session_1 = lzt::debug_attach(device1, debug_config);
  if (!debug_session_1) {
    lzt::debug_detach(debug_session_0);
    FAIL() << "[Debugger] Failed to attach to start a debug session";
  }

  LOG_DEBUG << "[Debugger] Notifying applications of attach";
  synchro->notify_application();

  uint64_t gpu_buffer_va_0 = 0, gpu_buffer_va_1 = 0;
  LOG_DEBUG << "[Debugger]  Spawning Threads";
  std::thread first_thread(wait_for_events_interrupt_and_resume,
                           debug_session_0, synchro, &gpu_buffer_va_0, device0);
  std::thread second_thread(wait_for_events_interrupt_and_resume,
                            debug_session_1, synchro, &gpu_buffer_va_1,
                            device1);

  synchro->wait_for_application_signal();
  if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va_0)) {
    FAIL()
        << "[Debugger] Could not get a valid GPU buffer VA for first device ";
  }

  synchro->clear_application_signal();
  synchro->notify_application();

  // get second gpu address
  synchro->wait_for_application_signal();
  if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va_1)) {
    FAIL() << "[Debugger] Could not get a valid GPU buffer VA for second "
              "device ";
  }
  synchro->clear_application_signal();
  address_valid = true;

  first_thread.join();
  second_thread.join();

  debugHelper.wait();
  LOG_INFO << "[Debugger] Debugger detaching";
  lzt::debug_detach(debug_session_0);
  lzt::debug_detach(debug_session_1);

  LOG_INFO << "[Debugger] Child debugger exited, finishing test";
  ASSERT_EQ(debugHelper.exit_code(), 0);
}

TEST_F(
    MultiDeviceDebugTest,
    GivenApplicationUsingTwoDevicesWhenDebuggingThenDebuggerCanAttachAndOperateOnBoth) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  auto initial_count = devices.size();
  devices.erase(std::remove_if(devices.begin(), devices.end(),
                               [](ze_device_handle_t &device) {
                                 return (!is_debug_supported(device));
                               }),
                devices.end());
  auto final_count = devices.size();

  if (final_count < 2) {
    LOG_WARNING << "Not enough "
                << ((final_count < initial_count) ? "debug capable " : "")
                << "devices to run multi-device test for driver";
  } else {
    run_multidevice_single_application_test(devices, false);
  }
}
