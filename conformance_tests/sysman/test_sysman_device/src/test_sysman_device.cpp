/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <boost/process.hpp>
#include <boost/filesystem.hpp>

namespace bp = boost::process;
namespace fs = boost::filesystem;

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

struct workload_thread_parameters {
  ze_device_handle_t device;
  std::mutex device_mutex;
  std::condition_variable condition;
  bool get_process_state_flag;
};

uint32_t get_prop_length(char prop[ZES_STRING_PROPERTY_SIZE]) {
  uint32_t length = 0;
  for (int i = 0; i < ZES_STRING_PROPERTY_SIZE; i++) {
    if (prop[i] == '\0') {
      break;
    } else {
      length += 1;
    }
  }

  return length;
}

#ifdef USE_ZESINIT
class SysmanDeviceZesTest : public lzt::ZesSysmanCtsClass {};
#define SYSMAN_DEVICE_TEST SysmanDeviceZesTest
#else // USE_ZESINIT
class SysmanDeviceTest : public lzt::SysmanCtsClass {};
#define SYSMAN_DEVICE_TEST SysmanDeviceTest
#endif // USE_ZESINIT

void run_device_hierarchy_child_process() {
  fs::path helper_path(fs::current_path() / "sysman_device");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  bp::ipstream child_output;
#ifdef USE_ZESINIT
  fs::path helper =
      bp::search_path("test_sysman_device_hierarchy_helper_zesinit", paths);
#else
  fs::path helper =
      bp::search_path("test_sysman_device_hierarchy_helper", paths);
#endif

  bp::child enumerate_devices_process(helper, bp::std_out > child_output);
  enumerate_devices_process.wait();

  LOG_INFO << "Child Process Logs";
  std::string result_string;
  while (std::getline(child_output, result_string)) {
    std::cout << result_string << "\n"; // Logs from Child Process
    if (result_string.find("Failure") != std::string::npos) {
      ADD_FAILURE() << "Test Case failed";
      return;
    }
  }

  ASSERT_EQ(enumerate_devices_process.exit_code(), 0);
}

#ifdef USE_ZESINIT
static void run_child_process(const std::string &device_hierarchy) {
  auto env = boost::this_process::environment();
  bp::environment child_env = env;
  child_env["ZE_FLAT_DEVICE_HIERARCHY"] = device_hierarchy;

  fs::path helper_path(fs::current_path() / "sysman_device");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  bp::ipstream child_output;
  fs::path helper = bp::search_path("test_sysman_device_helper_zesinit", paths);

  bp::child validate_deviceUUID_process(helper, child_env,
                                        bp::std_out > child_output);
  validate_deviceUUID_process.wait();
  std::cout << std::endl;
  std::string result_string;
  while (std::getline(child_output, result_string)) {
    std::cout << result_string << "\n"; // Display logs from Child Process
    if (result_string.find("Failure") != std::string::npos) {
      ADD_FAILURE() << "Test Case failed";
      return;
    }
  }
  ASSERT_EQ(validate_deviceUUID_process.exit_code(), 0);
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenHierarchyModeCombindedAndSysmanEnableEnvDisabledThenUUIDFromCoreAndSysmanMatches) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (strcmp(is_sysman_enabled, "1") == 0) {
    putenv("ZES_ENABLE_SYSMAN=0");
  }
  run_child_process("COMBINED");
  if (strcmp(is_sysman_enabled, "1") == 0) {
    putenv("ZES_ENABLE_SYSMAN=1");
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenHierarchyModeCompositeAndSysmanEnableEnvDisabledThenUUIDFromCoreAndSysmanMatches) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (strcmp(is_sysman_enabled, "1") == 0) {
    putenv("ZES_ENABLE_SYSMAN=0");
  }
  run_child_process("COMPOSITE");
  if (strcmp(is_sysman_enabled, "1") == 0) {
    putenv("ZES_ENABLE_SYSMAN=1");
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenHierarchyModeFlatAndSysmanEnableEnvDisabledThenUUIDFromCoreAndSysmanMatches) {
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  // Disabling enable_sysman env if it's defaultly enabled
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char disable_sysman_env[] = "ZES_ENABLE_SYSMAN=0";
    putenv(disable_sysman_env);
  }
  run_child_process("FLAT");
  if (is_sysman_enabled != nullptr && strcmp(is_sysman_enabled, "1") == 0) {
    char enable_sysman_env[] = "ZES_ENABLE_SYSMAN=1";
    putenv(enable_sysman_env);
  }
}
#endif // USE_ZESINIT

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenDeviceHierarchyModesFlatAndCombinedWhenRetrievingSysmanDevicesThenValidDevicesAreReturned) {
  run_device_hierarchy_child_process();
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingSubDevicePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto device_properties = lzt::get_sysman_device_properties(device);
    uint32_t sub_devices_count = device_properties.numSubdevices;

    if (sub_devices_count > 0) {
      uint32_t num_sub_devices = 0;
      auto sub_device_properties =
          lzt::get_sysman_subdevice_properties(device, num_sub_devices);
      EXPECT_EQ(sub_devices_count, num_sub_devices);
      for (uint32_t sub_device_index = 0; sub_device_index < num_sub_devices;
           sub_device_index++) {
        EXPECT_LT(sub_device_properties[sub_device_index].subdeviceId,
                  num_sub_devices);
        EXPECT_GT(sub_device_properties[sub_device_index].uuid.id[0], 0);
      }
    }
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDriverWhenRetrievingDeviceHandleFromUUIDThenReturnedDeviceHandleShouldMatchCurrentDeviceHandle) {
  zes_driver_handle_t driver = lzt::get_default_zes_driver();
  for (auto device : devices) {
    auto properties = lzt::get_sysman_device_properties(device);
    uint32_t sub_devices_count = properties.numSubdevices;
    zes_uuid_t uuid = {};
    for (uint32_t i = 0; i < sizeof(properties.core.uuid.id); i++) {
      uuid.id[i] = properties.core.uuid.id[i];
    }
    ze_bool_t on_sub_device = false;
    uint32_t sub_device_id = 0;
    zes_device_handle_t device_handle_from_uuid =
        lzt::get_sysman_device_by_uuid(driver, uuid, on_sub_device,
                                       sub_device_id);
    EXPECT_EQ(device_handle_from_uuid, device);
    if (on_sub_device == true) {
      EXPECT_LT(sub_device_id, sub_devices_count);
    }
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto properties = lzt::get_sysman_device_properties(device);

    EXPECT_GE(ZE_DEVICE_TYPE_GPU, properties.core.type);
    EXPECT_LE(properties.core.type, ZE_DEVICE_TYPE_MCA);
    if (properties.core.flags <=
        (ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
         ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
         ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING)) {
      if (properties.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_LT(properties.core.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_LE(get_prop_length(properties.serialNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.boardNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.brandName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.modelName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.vendorName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.driverVersion),
              ZES_STRING_PROPERTY_SIZE);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    auto properties_initial = lzt::get_sysman_device_properties(device);
    auto properties_later = lzt::get_sysman_device_properties(device);
    EXPECT_EQ(properties_initial.core.type, properties_later.core.type);
    if ((properties_initial.core.flags <=
         (ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
          ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
          ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING)) &&
        (properties_later.core.flags <=
         (ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
          ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
          ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING))) {
      EXPECT_EQ(properties_initial.core.flags, properties_later.core.flags);
      if (properties_initial.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE &&
          properties_later.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_EQ(properties_initial.core.subdeviceId,
                  properties_later.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.serialNumber,
                            properties_later.serialNumber,
                            get_prop_length(properties_initial.serialNumber)));
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.boardNumber,
                            properties_later.boardNumber,
                            get_prop_length(properties_initial.boardNumber)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.brandName,
                            properties_later.brandName,
                            get_prop_length(properties_initial.brandName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.modelName,
                            properties_later.modelName,
                            get_prop_length(properties_initial.modelName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.vendorName,
                            properties_later.vendorName,
                            get_prop_length(properties_initial.vendorName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.driverVersion,
                            properties_later.driverVersion,
                            get_prop_length(properties_initial.driverVersion)));
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenSuccessIsReturned) {
  for (auto device : devices) {
    lzt::get_processes_count(device);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenValidProcessesStateAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto processes = lzt::get_processes_state(device, count);
    if (processes.size() > 0) {
      for (auto process : processes) {
        EXPECT_GT(process.processId, 0u);
        EXPECT_GE(process.memSize, 0u);
        EXPECT_LT(process.sharedSize, UINT64_MAX);
        EXPECT_GE(process.engines, 0);
        EXPECT_LE(process.engines, (1 << ZES_ENGINE_TYPE_FLAG_DMA));
      }
    }
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceHandleWhenRetrievingProcessesStateThenProcessIdOfCallingProcessIsPresent) {
  for (auto device : devices) {
    uint32_t count = 0;
    int pid = getpid();
    int pid_found = 0;
    auto processes = lzt::get_processes_state(device, count);
    if (processes.size() > 0) {
      for (auto process : processes) {
        EXPECT_GT(process.processId, 0u);
        if (process.processId == pid) {
          pid_found = 1;
        }
      }
      EXPECT_EQ(pid_found, 1u);
    }
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_processes_state(device, actual_count);
    uint32_t test_count = actual_count + 1;
    lzt::get_processes_state(device, test_count);
    EXPECT_GE(test_count, actual_count);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingSysmanDeviceStateThenValidStateIsReturned) {
  for (auto device : devices) {
    auto state = lzt::get_device_state(device);
    EXPECT_GE(state.reset, 0);
    EXPECT_LE(state.reset,
              ZES_RESET_REASON_FLAG_WEDGED | ZES_RESET_REASON_FLAG_REPAIR);
    EXPECT_GE(state.repaired, ZES_REPAIR_STATUS_UNSUPPORTED);
    EXPECT_LE(state.repaired, ZES_REPAIR_STATUS_PERFORMED);
  }
}

TEST_F(SYSMAN_DEVICE_TEST,
       GivenValidDeviceWhenResettingSysmanDeviceThenSysmanDeviceResetSucceeds) {
  for (auto device : devices) {
    lzt::sysman_device_reset(device);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenResettingSysmanDeviceNnumberOfTimesThenSysmanDeviceResetAlwaysSucceeds) {
  int number_iterations = 2;
  for (int i = 0; i < number_iterations; i++) {
    for (auto device : devices) {
      lzt::sysman_device_reset(device);
    }
  }
}

static std::vector<float>
submit_workload_for_gpu(std::vector<float> a, std::vector<float> b,
                        uint32_t dim, ze_device_handle_t targetDevice) {
  int m, k, n;
  m = k = n = dim;
  std::vector<float> c(m * n, 0);
  ze_device_handle_t device = targetDevice;

  void *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  void *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  void *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));
  ze_module_handle_t module =
      lzt::create_module(device, "sysman_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "sysman_matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(a_buffer), &a_buffer);
  lzt::set_argument_value(function, 1, sizeof(b_buffer), &b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(c_buffer), &c_buffer);
  ze_command_list_handle_t cmd_list = lzt::create_command_list(device);
  std::memcpy(a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(b_buffer, b.data(), b.size() * sizeof(float));
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  const int group_count_x = m / 16;
  const int group_count_y = n / 16;
  ze_group_count_t tg;
  tg.groupCountX = group_count_x;
  tg.groupCountY = group_count_y;
  tg.groupCountZ = 1;
  zeCommandListAppendLaunchKernel(cmd_list, function, &tg, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_list);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(device);
  lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
  lzt::synchronize(cmd_q, UINT64_MAX);
  std::memcpy(c.data(), c_buffer, c.size() * sizeof(float));

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
  lzt::destroy_module(module);

  return c;
}

static std::vector<float>
perform_matrix_multiplication_on_cpu(std::vector<float> a, std::vector<float> b,
                                     uint32_t n) {
  std::vector<float> c_cpu(n * n, 0);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      float sum = 0;
      for (int kl = 0; kl < n; kl++) {
        sum += a[i * n + kl] * b[kl * n + j];
      }
      c_cpu[i * n + j] = sum;
    }
  }
  return c_cpu;
}

static void compare_results(std::vector<float> c, std::vector<float> c_cpu) {
  for (int i = 0; i < c.size(); i++) {
    if (c[i] != c_cpu[i]) {
      EXPECT_EQ(c[i], c_cpu[i]);
    }
  }
  return;
}

#ifdef USE_ZESINIT
bool is_uuids_equal(uint8_t *uuid1, uint8_t *uuid2) {
  for (uint32_t i = 0; i < ZE_MAX_UUID_SIZE; i++) {
    if (uuid1[i] != uuid2[i]) {
      return false;
    }
  }
  return true;
}
ze_device_handle_t get_core_device_by_uuid(uint8_t *uuid) {
  lzt::initialize_core();
  auto driver = lzt::zeDevice::get_instance()->get_driver();
  auto core_devices = lzt::get_ze_devices(driver);
  for (auto device : core_devices) {
    auto device_properties = lzt::get_device_properties(device);
    if (is_uuids_equal(uuid, device_properties.uuid.id)) {
      return device;
    }
  }
  return nullptr;
}
#endif // USE_ZESINIT

void compute_workload(workload_thread_parameters *t_params) {

  int m, k, n;
  m = k = n = 512;
  uint32_t count = 0;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);

  ze_device_handle_t device = t_params->device;
  void *a_buffer = lzt::allocate_device_memory(
      m * k * sizeof(float), 1, 0, device, lzt::get_default_context());
  void *b_buffer = lzt::allocate_device_memory(
      k * n * sizeof(float), 1, 0, device, lzt::get_default_context());
  void *c_buffer = lzt::allocate_device_memory(
      m * n * sizeof(float), 1, 0, device, lzt::get_default_context());
  ze_module_handle_t module =
      lzt::create_module(device, "sysman_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "sysman_matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(a_buffer), &a_buffer);
  lzt::set_argument_value(function, 1, sizeof(b_buffer), &b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(c_buffer), &c_buffer);
  ze_command_list_handle_t cmd_list = lzt::create_command_list(device);
  lzt::append_memory_copy(cmd_list, a_buffer, a.data(), lzt::size_in_bytes(a),
                          nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::append_memory_copy(cmd_list, b_buffer, b.data(), lzt::size_in_bytes(b),
                          nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);

  const int group_count_x = m / 16;
  const int group_count_y = n / 16;
  ze_group_count_t tg;
  tg.groupCountX = group_count_x;
  tg.groupCountY = group_count_y;
  tg.groupCountZ = 1;

  zeCommandListAppendLaunchKernel(cmd_list, function, &tg, nullptr, 0, nullptr);
  lzt::append_barrier(cmd_list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_list);
  ze_command_queue_handle_t cmd_q = lzt::create_command_queue(device);

  uint32_t number_iterations = 100;
  while (number_iterations--) {
    lzt::execute_command_lists(cmd_q, 1, &cmd_list, nullptr);
    lzt::synchronize(cmd_q, UINT64_MAX);
    t_params->get_process_state_flag = true;
    t_params->condition.notify_one();
  }

  lzt::destroy_command_queue(cmd_q);
  lzt::destroy_command_list(cmd_list);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
  lzt::destroy_module(module);
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingProcessesStateAndDeviceMemoryIsAllocatedThenMemorySizeIsReturnedCorrectly) {

  constexpr size_t sampleMemorySize = 512;

  for (auto device : devices) {
    uint32_t count = 0;
    uint64_t preAllocDeviceMemorySize = 0;
    uint64_t postAllocDeviceMemorySize = 0;
    uint64_t postFreeDeviceMemorySize = 0;

#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
#endif

    auto processes = lzt::get_processes_state(device, count);
    for (auto process : processes) {
      preAllocDeviceMemorySize += process.memSize;
    }
    processes.clear();

#ifdef USE_ZESINIT
    void *ptr = lzt::allocate_device_memory(sampleMemorySize, 1, 0, core_device,
                                            lzt::get_default_context());
#else  // USE_ZESINIT
    void *ptr = lzt::allocate_device_memory(sampleMemorySize);
#endif // USE_ZESINIT

    processes = lzt::get_processes_state(device, count);
    for (auto process : processes) {
      postAllocDeviceMemorySize += process.memSize;
    }
    processes.clear();
    EXPECT_GT(postAllocDeviceMemorySize, preAllocDeviceMemorySize);
    lzt::free_memory(ptr);

    processes = lzt::get_processes_state(device, count);
    for (auto process : processes) {
      postFreeDeviceMemorySize += process.memSize;
    }
    processes.clear();
    EXPECT_LT(postFreeDeviceMemorySize, postAllocDeviceMemorySize);
    EXPECT_EQ(preAllocDeviceMemorySize, postFreeDeviceMemorySize);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenValidDeviceWhenRetrievingProcessesStateAndComputeWorkloadIsExecutedThenEngineTypeUsedByProcessIsCorrect) {

  for (auto device : devices) {

    workload_thread_parameters t_params;
    t_params.get_process_state_flag = false;

#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    t_params.device = core_device;
    std::thread thread(compute_workload, &t_params);
#else  // USE_ZESINIT
    t_params.device = device;
    std::thread thread(compute_workload, &t_params);
#endif // USE_ZESINIT

    uint32_t count = 0;
    std::unique_lock<std::mutex> locker(t_params.device_mutex);
    t_params.condition.wait(locker,
                            [&] { return t_params.get_process_state_flag; });

    uint32_t engine_type = 0;
    while (engine_type != ZES_ENGINE_TYPE_FLAG_COMPUTE) {
      auto processes = lzt::get_processes_state(device, count);
      for (auto process : processes) {
        engine_type = process.engines;
      }
      processes.clear();
    }

    thread.join();
    EXPECT_EQ(engine_type, ZES_ENGINE_TYPE_FLAG_COMPUTE);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenWorkingDeviceHandleWhenResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterReset) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);

  for (auto device : devices) {
    // Perform workload execution before reset
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
    LOG_INFO << "Initiating device reset...\n";
    // perform device reset
    lzt::sysman_device_reset(device);
    LOG_INFO << "End of device reset...\n";

    // Perform workload execution after reset
#ifdef USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
  }
}
TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenWorkingDeviceHandleWhenResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterResetInMultipleIterations) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);
  const char *valueString = std::getenv("LZT_SYSMAN_DEVICE_TEST_ITERATIONS");
  uint32_t number_iterations = 2;
  if (valueString != nullptr) {
    auto _value = atoi(valueString);
    number_iterations = _value < 0 ? number_iterations : std::min(_value, 300);
    if (number_iterations != _value) {
      LOG_WARNING << "Number of iterations is capped at 300\n";
    }
  }

  for (auto device : devices) {
    for (int i = 0; i < number_iterations; i++) {
      // Perform workload execution before reset
#ifdef USE_ZESINIT
      auto sysman_device_properties = lzt::get_sysman_device_properties(device);
      ze_device_handle_t core_device =
          get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
      EXPECT_NE(core_device, nullptr);
      c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
      c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

      compare_results(c, c_cpu);
      c.clear();
      LOG_INFO << "Initiating device reset...\n";
      // perform device reset
      lzt::sysman_device_reset(device);
      LOG_INFO << "End of device reset...\n";

      // Perform workload execution after reset
#ifdef USE_ZESINIT
      c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
      c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

      compare_results(c, c_cpu);
      c.clear();
    }
  }
}

TEST_F(SYSMAN_DEVICE_TEST,
       GivenValidDeviceWhenWarmResettingDeviceThenDeviceResetExtSucceeds) {
  for (auto device : devices) {
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_WARM);
  }
}

TEST_F(SYSMAN_DEVICE_TEST,
       GivenValidDeviceWhenColdResettingDeviceThenDeviceResetExtSucceeds) {
  for (auto device : devices) {
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_COLD);
  }
}

TEST_F(SYSMAN_DEVICE_TEST,
       GivenValidDeviceWhenFlrResettingDeviceThenDeviceResetExtSucceeds) {
  for (auto device : devices) {
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_FLR);
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenWorkingDeviceHandleWhenWarmResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterResetExt) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);

  for (auto device : devices) {
    // Perform workload execution before reset
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
    LOG_INFO << "Initiating device reset...\n";
    // perform device reset
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_WARM);
    LOG_INFO << "End of device reset...\n";

    // Perform workload execution after reset
#ifdef USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenWorkingDeviceHandleWhenColdResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterResetExt) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);

  for (auto device : devices) {
    // Perform workload execution before reset
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
    LOG_INFO << "Initiating device reset...\n";
    // perform device reset
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_COLD);
    LOG_INFO << "End of device reset...\n";

    // Perform workload execution after reset
#ifdef USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
  }
}

TEST_F(
    SYSMAN_DEVICE_TEST,
    GivenWorkingDeviceHandleWhenFlrResettingSysmanDeviceThenWorkloadExecutionAlwaysSucceedsAfterResetExt) {
  uint32_t n = 512;
  std::vector<float> a(n * n, 1);
  std::vector<float> b(n * n, 1);
  std::vector<float> c;
  std::vector<float> c_cpu;
  c_cpu = perform_matrix_multiplication_on_cpu(a, b, n);

  for (auto device : devices) {
    // Perform workload execution before reset
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
    LOG_INFO << "Initiating device reset...\n";
    // perform device reset
    lzt::sysman_device_reset_ext(device, false, ZES_RESET_TYPE_FLR);
    LOG_INFO << "End of device reset...\n";

    // Perform workload execution after reset
#ifdef USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, core_device);
#else  // USE_ZESINIT
    c = submit_workload_for_gpu(a, b, n, device);
#endif // USE_ZESINIT

    compare_results(c, c_cpu);
    c.clear();
  }
}
} // namespace
