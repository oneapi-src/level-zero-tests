/*
 *
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_HPP
#define TEST_DEBUG_HPP

#include "test_debug_common.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

const uint32_t eventsTimeoutMS = 120000;
const uint16_t eventsTimeoutS = 30;

namespace lzt = level_zero_tests;

typedef enum { DEBUG, INFO, WARNING } log_level_t;

class ProcessLauncher {
public:
  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           const char *module_name, std::string module_options,
                           uint64_t index, bool use_many_threads) {

    std::string device_id = " ";
    if (device) {
      auto device_properties = lzt::get_device_properties(device);
      device_id = lzt::to_string(device_properties.uuid);
    }
    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path(bin_name, paths);

    auto optionize = [](const char *option, std::string value) {
      std::string option_string = " ";
      if (!value.empty()) {
        option_string = "--" + std::string(option) + "=" + value;
      } else {
        option_string = "--" + std::string(option);
      }
      return option_string;
    };

    std::string module_name_option = " ";
    if (module_name && module_name[0] != '\0') {
      module_name_option = optionize(module_string, module_name);
    }
    std::string module_build_options = " ";
    if (!module_options.empty()) {
      module_build_options = optionize(module_options_string, module_options);
    }
    bp::child debug_helper(
        helper, optionize(test_type_string, std::to_string(test_type)),

        device ? optionize(device_id_string, device_id) : " ",
        module_name_option, module_build_options,
        (use_sub_devices ? optionize(use_sub_devices_string, "") : " "),
        optionize(index_string, std::to_string(index)),
        (use_many_threads ? optionize(use_many_threads_string, "") : " "));

    return debug_helper;
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           const char *module_name, uint64_t index,
                           bool use_many_threads) {
    return launch_process(test_type, device, use_sub_devices, module_name, "",
                          index, use_many_threads);
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           const char *module_name,
                           std::string module_options) {
    return launch_process(test_type, device, use_sub_devices, module_name,
                          module_options, 0, false);
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           const char *module_name) {
    return launch_process(test_type, device, use_sub_devices, module_name, "",
                          0, false);
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices,
                           bool use_many_threads) {
    return launch_process(test_type, device, use_sub_devices, "", "", 0,
                          use_many_threads);
  }

  bp::child launch_process(debug_test_type_t test_type,
                           ze_device_handle_t device, bool use_sub_devices) {
    return launch_process(test_type, device, use_sub_devices, "", "", 0, false);
  }

protected:
  std::string bin_name = "test_debug_helper";
};

class zetDebugBaseSetup : public ProcessLauncher, public ::testing::Test {
protected:
  void SetUp() override {
    synchro = new process_synchro(true, true);

    auto driver = lzt::get_default_driver();
    auto devices = lzt::get_devices(driver);

    auto debug_capable_device = false;
    for (auto device : devices) {
      if (is_debug_supported(device)) {
        debug_capable_device = true;
        break;
      }
    }
    if (!debug_capable_device) {
      FAIL() << "[Debugger] No debug capable device found";
    }

    const testing::TestInfo *const test_info =
        testing::UnitTest::GetInstance()->current_test_info();
    std::string test_name =
        test_info && test_info->name() ? test_info->name() : "";
    std::transform(test_name.begin(), test_name.end(), test_name.begin(),
                   ::tolower);
    if (test_name.find("subdevice") != std::string::npos) {
      auto subdevices = lzt::get_all_sub_devices();
      if (!subdevices.empty()) {
        if (!std::all_of(subdevices.begin(), subdevices.end(),
                         is_debug_supported)) {
          FAIL() << "[Debugger] Subdevice does not support debug";
        }
      } else {
        GTEST_SKIP()
            << "Device does not support subdevices, skipping subdevice test";
      }
    }
    auto one_module_event_per_kernel_string =
        getenv("LZT_DEBUG_ONE_MODULE_EVENT_PER_KERNEL");
    if (one_module_event_per_kernel_string) {
      auto temp = std::stoi(one_module_event_per_kernel_string);
      if (temp == 1) {
        one_event_per_kernel = true;
      } else if (temp == 0) {
        one_event_per_kernel = false;
      }
    }
  }

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
  bool one_event_per_kernel = false;
};

class zetDebugAttachDetachTest : public zetDebugBaseSetup {
protected:
  void run_test(std::vector<ze_device_handle_t> &devices, bool use_sub_devices,
                bool reattach);
  void run_multidevice_test(std::vector<ze_device_handle_t> &devices,
                            bool use_sub_devices);
  void run_attach_detach_to_multiple_applications_on_different_devs_test(
      std::vector<ze_device_handle_t> &devices, bool use_sub_devices);
  void run_use_same_device_test(const zet_device_handle_t &device,
                                bool use_sub_devices);
  void
  run_new_debugger_attach_test(const std::vector<ze_device_handle_t> &devices,
                               bool verify_events);
};

class zetDebugMemAccessTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void run_module_isa_elf_test(std::vector<ze_device_handle_t> &devices,
                               bool use_sub_devices);
  void run_read_write_module_and_memory_test(
      std::vector<ze_device_handle_t> &devices, bool test_slm,
      uint64_t slmBaseAddress, bool use_sub_devices);
};

class MultiDeviceDebugTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void run_multidevice_single_application_test(
      std::vector<ze_device_handle_t> &devices, bool use_sub_devices);
  void run_multidevice_single_application_separate_debuggers_test(
      std::vector<ze_device_handle_t> &devices, bool use_sub_devices);
};

class zetDebugEventReadTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void run_test(std::vector<ze_device_handle_t> &devices, bool use_sub_devices,
                debug_test_type_t test_type);

  void run_attach_after_module_created_destroyed_test(
      std::vector<ze_device_handle_t> &devices, bool use_sub_devices,
      debug_test_type_t test_type);

  void
  run_multithreaded_application_test(std::vector<zet_device_handle_t> &devices,
                                     bool use_sub_devices);

  void run_read_events_in_separate_thread_test(
      std::vector<zet_device_handle_t> &devices, bool use_sub_devices);

  void run_proc_entry_exit_test(std::vector<zet_device_handle_t> &devices,
                                bool use_sub_devices,
                                debug_test_type_t test_type);
  void
  run_detach_no_ack_module_create_test(std::vector<ze_device_handle_t> &devices,
                                       bool use_sub_devices);
};

class zetDebugReadWriteRegistersTest : public zetDebugMemAccessTest {
protected:
  void SetUp() override { zetDebugMemAccessTest::SetUp(); }
  void TearDown() override { zetDebugMemAccessTest::TearDown(); }
  void run_read_write_registers_test(std::vector<ze_device_handle_t> &devices,
                                     bool use_sub_devices);
};

class zetDebugThreadControlTest : public zetDebugBaseSetup {
protected:
  void SetUp() override { zetDebugBaseSetup::SetUp(); }
  void TearDown() override { zetDebugBaseSetup::TearDown(); }
  void SetUpThreadControl(ze_device_handle_t &device, bool use_sub_devices,
                          bool use_many_threads = false);
  void run_alternate_stop_resume_test(std::vector<ze_device_handle_t> &devices,
                                      bool use_sub_devices);
  void run_interrupt_resume_test(std::vector<ze_device_handle_t> &devices,
                                 bool use_sub_devices,
                                 bool use_many_threads = false);
  void run_unavailable_thread_test(std::vector<ze_device_handle_t> &devices,
                                   bool use_sub_devices);
  void run_interrupt_and_resume_device_threads_in_separate_host_threads_test(
      std::vector<ze_device_handle_t> &devices, bool use_sub_devices);

  zet_debug_memory_space_desc_t memorySpaceDesc;
  std::vector<ze_device_thread_t> stopped_threads;

  // Order matters, ALL should go last
  typedef enum {
    SINGLE_THREAD,
    THREADS_IN_EU,
    THREADS_IN_SUBSLICE,
    THREADS_IN_SLICE,
    ALL_THREADS
  } threads_test_type_t;

  std::map<threads_test_type_t, std::string> numThreadsToString = {
      {SINGLE_THREAD, "SINGLE_THREAD"},
      {THREADS_IN_EU, "THREADS_IN_EU"},
      {THREADS_IN_SUBSLICE, "THREADS_IN_SUBSLICE"},
      {THREADS_IN_SLICE, "THREADS_IN_SLICE"},
      {ALL_THREADS, "ALL_THREADS"}};
};

#endif // TEST_DEBUG_HPP
