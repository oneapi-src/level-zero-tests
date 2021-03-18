/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

enum TestType { FUNCTION, FUNCTION_INDIRECT, MULTIPLE_INDIRECT };

struct FunctionData {
  void *host_buffer;
  void *shared_buffer;
};

std::vector<ze_module_handle_t> create_module_vector_and_log(
    ze_device_handle_t device, const std::string filename_prefix,
    std::vector<ze_module_build_log_handle_t> *build_log) {
  std::vector<ze_module_handle_t> module;

  std::vector<const char *> build_flag = {
      nullptr, "-ze-opt-disable", "-ze-opt-greater-than-4GB-buffer-required",
      "-ze-opt-large-register-file"};

  auto start = std::chrono::system_clock::now();
  auto end = std::chrono::system_clock::now();
  // Create pseudo-random integer to add to native binary filename
  srand(time(NULL) +
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count());
  std::string filename_native =
      filename_prefix + std::to_string(rand()) + ".native";
  std::string filename_spirv = filename_prefix + ".spv";

  if (build_log) {
    build_log->resize(2 * build_flag.size());
    size_t count = 0;

    for (auto flag : build_flag) {

      module.push_back(lzt::create_module(device, filename_spirv,
                                          ZE_MODULE_FORMAT_IL_SPIRV, flag,
                                          &build_log->at(count)));
      count++;

      lzt::save_native_binary_file(module.back(), filename_native);
      module.push_back(lzt::create_module(device, filename_native,
                                          ZE_MODULE_FORMAT_NATIVE, nullptr,
                                          &build_log->at(count)));
      count++;
      std::remove(filename_native.c_str());
    }
  } else {
    for (auto flag : build_flag) {
      module.push_back(lzt::create_module(
          device, filename_spirv, ZE_MODULE_FORMAT_IL_SPIRV, flag, nullptr));

      lzt::save_native_binary_file(module.back(), filename_native);
      module.push_back(lzt::create_module(
          device, filename_native, ZE_MODULE_FORMAT_NATIVE, nullptr, nullptr));
      std::remove(filename_native.c_str());
    }
  }
  return (module);
}

std::vector<ze_module_handle_t>
create_module_vector(ze_device_handle_t device,
                     const std::string filename_prefix) {
  return (create_module_vector_and_log(device, filename_prefix, nullptr));
}
class zeModuleCreateTests : public ::testing::Test {};

// TODO: Dynamically Link module for these tests to be valid
TEST_F(
    zeModuleCreateTests,
    GivenModuleWithGlobalVariableWhenRetrievingGlobalPointerThenPointerPointsToValidGlobalVariable) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "single_global_variable");

  const std::string global_name = "global_variable";
  void *global_pointer;
  const int expected_value = 123;
  int *typed_global_pointer;

  for (auto mod : module) {
    global_pointer = nullptr;
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeModuleGetGlobalPointer(mod, global_name.c_str(), nullptr,
                                       &global_pointer));
    EXPECT_NE(nullptr, global_pointer);
    typed_global_pointer = static_cast<int *>(global_pointer);
    EXPECT_EQ(expected_value, *typed_global_pointer);
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    WhenRetrievingMultipleGlobalPointersFromTheSameVariableThenAllPointersAreTheSame) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "single_global_variable");

  const std::string global_name = "global_variable";
  void *previous_pointer;
  void *current_pointer;

  for (auto mod : module) {
    previous_pointer = nullptr;

    for (int i = 0; i < 5; ++i) {
      current_pointer = nullptr;
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                zeModuleGetGlobalPointer(mod, global_name.c_str(), nullptr,
                                         &current_pointer));
      EXPECT_NE(nullptr, current_pointer);

      if (i > 0) {
        EXPECT_EQ(previous_pointer, current_pointer);
      }
      previous_pointer = current_pointer;
    }
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    GivenModuleWithMultipleGlobalVariablesWhenRetrievingGlobalPointersThenAllPointersPointToValidGlobalVariable) {

  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "multiple_global_variables");

  const int global_count = 5;
  void *global_pointer;
  int *typed_global_pointer;

  for (auto mod : module) {
    for (int i = 0; i < global_count; ++i) {
      std::string global_name = "global_" + std::to_string(i);
      global_pointer = nullptr;
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                zeModuleGetGlobalPointer(mod, global_name.c_str(), nullptr,
                                         &global_pointer));
      EXPECT_NE(nullptr, global_pointer);

      typed_global_pointer = static_cast<int *>(global_pointer);
      EXPECT_EQ(i, *typed_global_pointer);
    }
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    GivenGlobalPointerWhenUpdatingGlobalVariableOnDeviceThenGlobalPointerPointsToUpdatedVariable) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "update_variable_on_device");

  const std::string global_name = "global_variable";
  void *global_pointer;
  int *typed_global_pointer;
  const int expected_initial_value = 1;
  const int expected_updated_value = 2;

  for (auto mod : module) {
    global_pointer = nullptr;
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeModuleGetGlobalPointer(mod, global_name.c_str(), nullptr,
                                       &global_pointer));
    EXPECT_NE(nullptr, global_pointer);
    typed_global_pointer = static_cast<int *>(global_pointer);
    EXPECT_EQ(expected_initial_value, *typed_global_pointer);
    lzt::create_and_execute_function(device, mod, "test", 1, nullptr);
    EXPECT_EQ(expected_updated_value, *typed_global_pointer);
    lzt::destroy_module(mod);
  }
}
TEST_F(
    zeModuleCreateTests,
    GivenValidSpecConstantsWhenCreatingModuleThenExpectSpecConstantInKernelGetsUpdates) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  ze_module_handle_t module =
      lzt::create_module(device, "update_variable_with_spec_constant.spv");
  std::string kernel_name = "test";
  void *buff = lzt::allocate_host_memory(sizeof(uint64_t));
  lzt::create_and_execute_function(device, module, kernel_name, 1, buff);
  uint64_t data = *static_cast<uint64_t *>(buff);
  const uint64_t expected_initial_value = 160;
  EXPECT_EQ(expected_initial_value, data);
  lzt::free_memory(buff);
  lzt::destroy_module(module);
  uint32_t specConstantsNum = 3;
  const uint64_t val_1 = 20, val_2 = 40, val_3 = 80;
  const uint64_t *specConstantsValues[] = {&val_1, &val_2, &val_3};
  uint32_t specConstantsIDs[] = {0, 1, 3};
  ze_module_constants_t specConstants{specConstantsNum, specConstantsIDs,
                                      (const void **)specConstantsValues};
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  ze_module_handle_t module_spec;
  const std::string filename = "update_variable_with_spec_constant.spv";
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file(filename);

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;
  module_description.pConstants = &specConstants;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeModuleCreate(lzt::get_default_context(), device,
                           &module_description, &module_spec, nullptr));
  kernel_name = "test";
  void *buff_spec = lzt::allocate_host_memory(sizeof(uint64_t));
  lzt::create_and_execute_function(device, module_spec, kernel_name, 1,
                                   buff_spec);
  data = *static_cast<uint64_t *>(buff_spec);
  const uint64_t expected_updated_value = 190;
  EXPECT_EQ(expected_updated_value, data);
  lzt::free_memory(buff_spec);
  lzt::destroy_module(module_spec);
}
TEST_F(
    zeModuleCreateTests,
    GivenModuleWithFunctionWhenRetrievingFunctionPointerThenPointerPointsToValidFunction) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "module_add");

  const std::string function_name = "module_add_constant";
  void *function_pointer;

  for (auto mod : module) {
    function_pointer = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeModuleGetFunctionPointer(mod, function_name.c_str(),
                                         &function_pointer));
    EXPECT_NE(nullptr, function_pointer);
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    GivenValidDeviceAndBinaryFileWhenCreatingModuleThenReturnSuccessfulAndDestroyModule) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_module_handle_t> module =
      create_module_vector(device, "module_add");
  for (auto mod : module) {
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    GivenValidDeviceAndBinaryFileWhenCreatingModuleThenOutputBuildLogAndReturnSuccessful) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  std::vector<ze_module_build_log_handle_t> build_log;
  std::vector<ze_module_handle_t> module =
      create_module_vector_and_log(device, "module_add", &build_log);

  size_t build_log_size;
  std::string build_log_str;

  for (auto log : build_log) {
    build_log_size = lzt::get_build_log_size(log);
    build_log_str = lzt::get_build_log_string(log);

    EXPECT_EQ(1, build_log_size);
    EXPECT_EQ('\0', build_log_str[0]);
    LOG_INFO << "Build Log Size = " << build_log_size;
    LOG_INFO << "Build Log String = " << build_log_str;

    lzt::destroy_build_log(log);
  }

  for (auto mod : module) {
    lzt::destroy_module(mod);
  }
}

TEST_F(
    zeModuleCreateTests,
    GivenInvalidDeviceAndBinaryFileWhenCreatingModuleThenFailsAndOutputBuildReturnsErrorString) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_module_build_log_handle_t build_log_error;
  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  size_t build_log_size;
  std::string build_log_str;
  ze_module_handle_t module_error = nullptr;
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file("module_build_error.spv");

  module_description.pNext = nullptr;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = nullptr;
  EXPECT_EQ(ZE_RESULT_ERROR_MODULE_BUILD_FAILURE,
            zeModuleCreate(lzt::get_default_context(), device,
                           &module_description, &module_error,
                           &build_log_error));
  EXPECT_EQ(nullptr, module_error);
  build_log_size = lzt::get_build_log_size(build_log_error);
  build_log_str = lzt::get_build_log_string(build_log_error);
  EXPECT_GT(build_log_size, 1);
  EXPECT_NE('\0', build_log_str[0]);
  LOG_INFO << "Build Log Size = " << build_log_size;
  LOG_INFO << "Build Log String = " << build_log_str;
}

TEST_F(
    zeModuleCreateTests,
    GivenValidModuleWhenGettingNativeBinaryFileThenRetrieveFileAndReturnSuccessful) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  size_t size = 0;
  ze_module_build_log_handle_t build_log;
  // Note: Only one example shown here, as subset of functionality of
  // "create_module_vector"

  auto start = std::chrono::system_clock::now();
  auto end = std::chrono::system_clock::now();
  // Create pseudo-random integer to add to native binary filename
  srand(time(NULL) +
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count());
  std::string filename_native =
      "module_add" + std::to_string(rand()) + ".native";
  std::string filename_spirv = "module_add.spv";

  ze_module_handle_t module =
      lzt::create_module(device, filename_spirv, ZE_MODULE_FORMAT_IL_SPIRV,
                         "-ze-opt-disable", nullptr);
  size = lzt::get_native_binary_size(module);
  LOG_INFO << "Native binary size: " << size;
  lzt::save_native_binary_file(module, filename_native);

  std::ifstream stream(filename_native, std::ios::in | std::ios::binary);
  stream.seekg(0, stream.end);
  EXPECT_EQ(static_cast<size_t>(stream.tellg()), size);
  std::remove(filename_native.c_str());
  lzt::destroy_module(module);
}

class zeKernelCreateTests : public lzt::zeEventPoolTests {
protected:
  void SetUp() override {
    device_ = lzt::zeDevice::get_instance()->get_device();
    module_ = create_module_vector(device_, "module_add");
  }

  void run_test(ze_module_handle_t mod, ze_group_count_t th_group_dim,
                uint32_t group_size_x, uint32_t group_size_y,
                uint32_t group_size_z, bool signal_to_host,
                bool signal_from_host, enum TestType type) {
    uint32_t num_events = std::min(group_size_x, static_cast<uint32_t>(6));
    ze_event_handle_t event_kernel_to_host = nullptr;
    ze_kernel_handle_t function;
    ze_kernel_handle_t mult_function;
    std::vector<ze_event_handle_t> events_host_to_kernel(num_events, nullptr);
    std::vector<int> inpa = {0, 1, 2,  3,  4,  5,  6,  7,
                             8, 9, 10, 11, 12, 13, 14, 15};
    std::vector<int> inpb = {1, 2,  3,  4,  5,  6,  7,  8,
                             9, 10, 11, 12, 13, 14, 15, 16};
    void *args_buff = lzt::allocate_shared_memory(2 * sizeof(ze_group_count_t),
                                                  sizeof(int), 0, 0, device_);
    void *actual_launch = lzt::allocate_shared_memory(
        sizeof(uint32_t), sizeof(uint32_t), 0, 0, device_);
    void *input_a =
        lzt::allocate_shared_memory(16 * sizeof(int), 1, 0, 0, device_);
    void *mult_out =
        lzt::allocate_shared_memory(16 * sizeof(int), 1, 0, 0, device_);
    void *mult_in =
        lzt::allocate_shared_memory(16 * sizeof(int), 1, 0, 0, device_);
    void *host_buff = lzt::allocate_host_memory(sizeof(int));
    int *host_addval_offset = static_cast<int *>(host_buff);

    const int addval = 10;
    const int host_offset = 200;
    int *input_a_int = static_cast<int *>(input_a);

    if (signal_to_host) {
      ep.create_event(event_kernel_to_host, ZE_EVENT_SCOPE_FLAG_HOST,
                      ZE_EVENT_SCOPE_FLAG_HOST);
    }
    if (signal_from_host) {
      ep.create_events(events_host_to_kernel, num_events,
                       ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST);
    }

    function = lzt::create_function(mod, "module_add_constant");
    ze_command_list_handle_t cmd_list = lzt::create_command_list(device_);
    ze_command_queue_handle_t cmd_q = lzt::create_command_queue(device_);
    memset(input_a, 0, 16);

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 0, sizeof(input_a_int),
                                       &input_a_int));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 1, sizeof(addval), &addval));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetGroupSize(function, group_size_x, group_size_y,
                                   group_size_z));

    ze_kernel_properties_t kernel_properties;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelGetProperties(function, &kernel_properties));

    EXPECT_EQ(kernel_properties.numKernelArgs, 2);

    ze_event_handle_t signal_event = nullptr;
    uint32_t num_wait = 0;
    ze_event_handle_t *p_wait_events = nullptr;
    if (signal_to_host) {
      signal_event = event_kernel_to_host;
    }
    if (signal_from_host) {
      p_wait_events = events_host_to_kernel.data();
      num_wait = num_events;
    }
    auto wait_events_initial = events_host_to_kernel;
    if (type == FUNCTION) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendLaunchKernel(
                                       cmd_list, function, &th_group_dim,
                                       signal_event, num_wait, p_wait_events));
    } else if (type == FUNCTION_INDIRECT) {
      ze_group_count_t *tg_dim = static_cast<ze_group_count_t *>(args_buff);
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendLaunchKernelIndirect(
                                       cmd_list, function, tg_dim, signal_event,
                                       num_wait, p_wait_events));

      // Intentionally update args_buff after Launch API
      memcpy(args_buff, &th_group_dim, sizeof(ze_group_count_t));
    } else if (type == MULTIPLE_INDIRECT) {
      int *mult_out_int = static_cast<int *>(mult_out);
      int *mult_in_int = static_cast<int *>(mult_in);
      ze_group_count_t *tg_dim = static_cast<ze_group_count_t *>(args_buff);
      memcpy(mult_out_int, inpa.data(), 16 * sizeof(int));
      memcpy(mult_in_int, inpb.data(), 16 * sizeof(int));
      std::vector<ze_kernel_handle_t> function_list;
      std::vector<ze_group_count_t> arg_buffer_list;
      std::vector<uint32_t> num_launch_arg_list;
      function_list.push_back(function);
      mult_function = lzt::create_function(mod, "module_add_two_arrays");
      function_list.push_back(mult_function);

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetArgumentValue(mult_function, 0, sizeof(mult_out_int),
                                         &mult_out_int));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetArgumentValue(mult_function, 1, sizeof(mult_in_int),
                                         &mult_in_int));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetGroupSize(mult_function, 16, 1, 1));

      arg_buffer_list.push_back(th_group_dim);
      ze_group_count_t mult_th_group_dim;
      mult_th_group_dim.groupCountX = 16;
      mult_th_group_dim.groupCountY = 1;
      mult_th_group_dim.groupCountZ = 1;
      arg_buffer_list.push_back(mult_th_group_dim);
      uint32_t *num_launch_arg = static_cast<uint32_t *>(actual_launch);
      ze_group_count_t *mult_tg_dim =
          static_cast<ze_group_count_t *>(args_buff);
      auto functions_initial = function_list;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendLaunchMultipleKernelsIndirect(
                    cmd_list, 2, function_list.data(), num_launch_arg,
                    mult_tg_dim, signal_event, num_wait, p_wait_events));
      for (int i = 0; i < function_list.size(); i++) {
        ASSERT_EQ(function_list[i], functions_initial[i]);
      }

      // Intentionally update args buffer and num_args after API
      num_launch_arg[0] = 2;
      memcpy(args_buff, arg_buffer_list.data(), 2 * sizeof(ze_group_count_t));
    }
    for (int i = 0; i < events_host_to_kernel.size(); i++) {
      EXPECT_EQ(events_host_to_kernel[i], wait_events_initial[i]);
    }
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendBarrier(cmd_list, nullptr, 0, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(cmd_list));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueExecuteCommandLists(cmd_q, 1, &cmd_list, nullptr));

    if (signal_from_host) {
      for (uint32_t i = 0; i < num_events; i++) {
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeEventHostSignal(events_host_to_kernel[i]));
      }
    }

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueSynchronize(cmd_q, UINT64_MAX));

    if (signal_to_host) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeEventHostSynchronize(event_kernel_to_host, UINT32_MAX - 1));
    }

    int offset = 0;

    EXPECT_EQ(input_a_int[0],
              offset + ((addval * (group_size_x * th_group_dim.groupCountX) *
                         (group_size_y * th_group_dim.groupCountY) *
                         (group_size_z * th_group_dim.groupCountZ))));
    if (type == MULTIPLE_INDIRECT) {
      int *mult_out_int = static_cast<int *>(mult_out);
      for (uint32_t i = 0; i < 16; i++) {
        EXPECT_EQ(mult_out_int[i], inpa[i] + inpb[i]);
      }
      lzt::destroy_function(mult_function);
    }
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueDestroy(cmd_q));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListDestroy(cmd_list));
    lzt::destroy_function(function);
    lzt::free_memory(host_buff);
    lzt::free_memory(mult_in);
    lzt::free_memory(mult_out);
    lzt::free_memory(input_a);
    lzt::free_memory(actual_launch);
    lzt::free_memory(args_buff);
    if (signal_to_host) {
      ep.destroy_event(event_kernel_to_host);
    }
    if (signal_from_host) {
      ep.destroy_events(events_host_to_kernel);
    }
  }

  void TearDown() override {
    for (auto mod : module_) {
      lzt::destroy_module(mod);
    }
  }

  ze_device_handle_t device_ = nullptr;
  std::vector<ze_module_handle_t> module_;
};

TEST_F(zeKernelCreateTests,
       GivenValidModuleWhenCreatingFunctionThenReturnSuccessful) {
  ze_kernel_handle_t function;

  for (auto mod : module_) {
    function = lzt::create_function(mod, 0, "module_add_constant");
    lzt::destroy_function(function);
    function = lzt::create_function(mod, ZE_KERNEL_FLAG_FORCE_RESIDENCY,
                                    "module_add_two_arrays");
    lzt::destroy_function(function);
  }
}

TEST_F(zeKernelCreateTests,
       GivenValidFunctionWhenSettingGroupSizeThenReturnSuccessful) {
  ze_kernel_handle_t function;
  ze_device_compute_properties_t dev_compute_properties;
  uint32_t x;
  uint32_t y;
  uint32_t z;

  for (auto mod : module_) {
    function = lzt::create_function(mod, 0, "module_add_constant");
    dev_compute_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetComputeProperties(device_, &dev_compute_properties));
    for (uint32_t x = 1; x < dev_compute_properties.maxGroupSizeX; x++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, x, 1, 1));
    }
    for (uint32_t y = 1; y < dev_compute_properties.maxGroupSizeY; y++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, 1, y, 1));
    }
    for (uint32_t z = 1; z < dev_compute_properties.maxGroupSizeZ; z++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, 1, 1, z));
    }
    x = y = z = 1;
    while (x * y < dev_compute_properties.maxTotalGroupSize) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, x++, y++, 1));
    }
    x = y = z = 1;
    while (y * z < dev_compute_properties.maxTotalGroupSize) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, 1, y++, z++));
    }
    x = y = z = 1;
    while (x * z < dev_compute_properties.maxTotalGroupSize) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelSetGroupSize(function, x++, 1, z++));
    }
    x = y = z = 1;
    while (x * y * z < dev_compute_properties.maxTotalGroupSize) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSetGroupSize(function, x++, y++, z++));
    }
    lzt::destroy_function(function);
  }
}

TEST_F(zeKernelCreateTests,
       GivenValidFunctionWhenSuggestingGroupSizeThenReturnSuccessful) {
  ze_kernel_handle_t function;
  ze_device_compute_properties_t dev_compute_properties;
  uint32_t group_size_x;
  uint32_t group_size_y;
  uint32_t group_size_z;

  for (auto mod : module_) {
    function = lzt::create_function(mod, 0, "module_add_constant");
    dev_compute_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetComputeProperties(device_, &dev_compute_properties));
    group_size_x = group_size_y = group_size_z = 0;
    for (uint32_t x = UINT32_MAX; x > 0; x = x >> 1) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSuggestGroupSize(function, x, 1, 1, &group_size_x,
                                         &group_size_y, &group_size_z));
      EXPECT_LE(group_size_x, dev_compute_properties.maxGroupSizeX);
    }
    for (uint32_t y = UINT32_MAX; y > 0; y = y >> 1) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSuggestGroupSize(function, 1, y, 1, &group_size_x,
                                         &group_size_y, &group_size_z));
      EXPECT_LE(group_size_y, dev_compute_properties.maxGroupSizeY);
    }
    for (uint32_t z = UINT32_MAX; z > 0; z = z >> 1) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSuggestGroupSize(function, 1, 1, z, &group_size_x,
                                         &group_size_y, &group_size_z));
      EXPECT_LE(group_size_z, dev_compute_properties.maxGroupSizeZ);
    }
    for (uint32_t i = UINT32_MAX; i > 0; i = i >> 1) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeKernelSuggestGroupSize(function, i, i, i, &group_size_x,
                                         &group_size_y, &group_size_z));
      EXPECT_LE(group_size_x, dev_compute_properties.maxGroupSizeX);
      EXPECT_LE(group_size_y, dev_compute_properties.maxGroupSizeY);
      EXPECT_LE(group_size_z, dev_compute_properties.maxGroupSizeZ);
    }
    lzt::destroy_function(function);
  }
}

TEST_F(zeKernelCreateTests,
       GivenValidFunctionWhenSettingArgumentsThenReturnSuccessful) {

  void *input_a = lzt::allocate_shared_memory(16, 1, 0, 0, device_);
  void *input_b = lzt::allocate_shared_memory(16, 1, 0, 0, device_);
  const int addval = 10;
  int *input_a_int = static_cast<int *>(input_a);
  int *input_b_int = static_cast<int *>(input_b);
  ze_kernel_handle_t function;

  for (auto mod : module_) {
    function = lzt::create_function(mod, "module_add_constant");
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 0, sizeof(input_a_int),
                                       &input_a_int));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 1, sizeof(addval), &addval));

    lzt::destroy_function(function);
    function = lzt::create_function(mod, "module_add_two_arrays");
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 0, sizeof(input_a_int),
                                       &input_a_int));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 1, sizeof(input_b_int),
                                       &input_b_int));
    lzt::destroy_function(function);
  }
  lzt::free_memory(input_a);
  lzt::free_memory(input_b);
}

TEST_F(
    zeKernelCreateTests,
    GivenValidFunctionWhenGettingPropertiesThenReturnSuccessfulAndPropertiesAreValid) {
  ze_device_compute_properties_t dev_compute_properties;

  dev_compute_properties.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetComputeProperties(device_, &dev_compute_properties));

  ze_kernel_handle_t function;
  uint32_t attribute_val;

  for (auto mod : module_) {
    function = lzt::create_function(mod, "module_add_constant");
    ze_kernel_properties_t kernel_properties;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelGetProperties(function, &kernel_properties));
    EXPECT_LE(kernel_properties.requiredGroupSizeX,
              dev_compute_properties.maxGroupCountX);
    EXPECT_LE(kernel_properties.requiredGroupSizeY,
              dev_compute_properties.maxGroupCountY);
    EXPECT_LE(kernel_properties.requiredGroupSizeZ,
              dev_compute_properties.maxGroupCountZ);

    LOG_INFO << "Num of Arguments = " << kernel_properties.numKernelArgs;
    LOG_INFO << "Group Size in X dim = "
             << kernel_properties.requiredGroupSizeX;
    LOG_INFO << "Group Size in Y dim = "
             << kernel_properties.requiredGroupSizeY;
    LOG_INFO << "Group Size in Z dim = "
             << kernel_properties.requiredGroupSizeZ;
    lzt::destroy_function(function);
  }
}

TEST_F(
    zeKernelCreateTests,
    DISABLED_GivenValidFunctionWhenGettingSourceAttributeThenReturnAttributeString) {
  ze_kernel_handle_t function;
  std::string kernel_source_attr = "work_group_size_hint(1,1,1)";
  kernel_source_attr.push_back('\0');
  for (auto mod : module_) {
    function = lzt::create_function(mod, "module_add_attr");
    char *get_source_attr_char = lzt::get_kernel_source_attribute(function);
    auto get_source_attr = std::string(get_source_attr_char);
    EXPECT_EQ(0, get_source_attr.compare(kernel_source_attr));
    lzt::destroy_function(function);
    free(get_source_attr_char);
  }
}

class zeKernelLaunchTests
    : public ::zeKernelCreateTests,
      public ::testing::WithParamInterface<enum TestType> {
protected:
  void test_kernel_execution();
};

void zeKernelLaunchTests::test_kernel_execution() {
  ze_device_compute_properties_t dev_compute_properties;

  dev_compute_properties.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetComputeProperties(device_, &dev_compute_properties));

  uint32_t group_size_x;
  uint32_t group_size_y;
  uint32_t group_size_z;
  ze_group_count_t thread_group_dimensions;

  enum TestType test_type = GetParam();

  std::vector<int> dim = {1, 2, 3};
  std::vector<uint32_t> tg_count = {1, 2, 3, 4};
  std::vector<uint32_t> grp_size = {1, 2, 3, 4};
  std::vector<bool> sig_to_host = {false, true};
  std::vector<bool> sig_from_host = {false, true};
  if (test_type == MULTIPLE_INDIRECT) {
    dim.erase(dim.begin(), dim.begin() + 2);
    tg_count.erase(tg_count.begin(), tg_count.begin() + 3);
    grp_size.erase(grp_size.begin(), grp_size.begin() + 3);
    sig_to_host.erase(sig_to_host.begin(), sig_to_host.begin() + 1);
    sig_from_host.erase(sig_from_host.begin(), sig_from_host.begin() + 1);
  }

  uint32_t count = 0;
  for (auto mod : module_) {
    LOG_INFO << "module count = " << count;
    count++;
    for (auto d : dim) {
      LOG_INFO << d << "-Dimensional Group Size Tests";
      for (auto tg : tg_count) {
        thread_group_dimensions.groupCountX = tg;
        thread_group_dimensions.groupCountY = (d > 1) ? tg : 1;
        thread_group_dimensions.groupCountZ = (d > 2) ? tg : 1;
        for (auto grp : grp_size) {
          group_size_x = grp;
          group_size_y = (d > 1) ? grp : 1;
          group_size_z = (d > 2) ? grp : 1;
          ASSERT_LE(group_size_x * group_size_y * group_size_z,
                    dev_compute_properties.maxTotalGroupSize);
          for (auto sig1 : sig_to_host) {
            for (auto sig2 : sig_from_host) {
              run_test(mod, thread_group_dimensions, group_size_x, group_size_y,
                       group_size_z, sig1, sig2, test_type);
            }
          }
        }
      }
    }
  }
}

TEST_P(
    zeKernelLaunchTests,
    GivenValidFunctionWhenAppendLaunchKernelThenReturnSuccessfulAndVerifyExecution) {
  test_kernel_execution();
}

INSTANTIATE_TEST_CASE_P(
    TestFunctionAndFunctionIndirectAndMultipleFunctionsIndirect,
    zeKernelLaunchTests,
    testing::Values(FUNCTION, FUNCTION_INDIRECT, MULTIPLE_INDIRECT));

TEST_F(
    zeKernelLaunchTests,
    GivenBufferLargerThan4GBWhenExecutingFunctionThenFunctionExecutesSuccessfully) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto context = lzt::create_context(driver);

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);

  auto module = lzt::create_module(
      context, device, "module_add.spv", ZE_MODULE_FORMAT_IL_SPIRV,
      "-ze-opt-greater-than-4GB-buffer-required", nullptr);
  auto kernel = lzt::create_function(module, "module_add_constant_2");

  auto device_properties = lzt::get_device_properties(device);

  auto driver_extension_properties = lzt::get_extension_properties(driver);
  bool supports_relaxed_allocations = false;
  for (auto &extension : driver_extension_properties) {
    if (!std::strcmp("ZE_experimental_relaxed_allocation_limits",
                     extension.name)) {
      supports_relaxed_allocations = true;
      break;
    }
  }

  if (!supports_relaxed_allocations) {
    LOG_WARNING
        << "size exceeds device max allocation size and driver does not "
           "support relaxed allocations, skipping test";
    return;
  }

  ze_relaxed_allocation_limits_exp_desc_t relaxed_allocation_limits_desc = {};
  relaxed_allocation_limits_desc.stype =
      ZE_STRUCTURE_TYPE_RELAXED_ALLOCATION_LIMITS_EXP_DESC;
  relaxed_allocation_limits_desc.flags =
      ZE_RELAXED_ALLOCATION_LIMITS_EXP_FLAG_MAX_SIZE;
  void *pNext = &relaxed_allocation_limits_desc;

  const auto head = 4096;
  auto size = device_properties.maxMemAllocSize + head;

  auto buffer_a = lzt::allocate_shared_memory(size, 0, 0, pNext, 0, nullptr,
                                              device, context);
  auto buffer_b =
      lzt::allocate_device_memory(size, 0, 0, pNext, 0, device, context);

  if (::testing::Test::HasFailure()) {
    FAIL() << "Error allocating memory";
  }

  std::memset(buffer_a, 0, size);
  for (size_t i = 0; i < size; i++) {
    static_cast<uint8_t *>(buffer_a)[i] = (i & 0xFF);
  }
  const int addval = 3;

  lzt::set_argument_value(kernel, 0, sizeof(buffer_b), &buffer_b);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);

  auto device_compute_properties = lzt::get_compute_properties(device);

  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  lzt::suggest_group_size(kernel, head, 1, 1, group_size_x, group_size_y,
                          group_size_z);
  if (group_size_x > device_compute_properties.maxGroupSizeX) {
    LOG_WARNING
        << "Suggested group size is larger than max group size, setting to max";
    group_size_x = device_compute_properties.maxGroupSizeX;
  }

  lzt::set_group_size(kernel, group_size_x, 1, 1);
  ze_group_count_t group_count = {};
  group_count.groupCountX = head / group_size_x;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::append_memory_copy(command_list, buffer_b, buffer_a, size);
  lzt::append_barrier(command_list);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, buffer_a, buffer_b, size);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(buffer_a)[i],
              (i >= head) ? static_cast<uint8_t>(i & 0xFF)
                          : static_cast<uint8_t>((i & 0xFF) + addval));
  }

  // cleanup
  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);
}

class zeKernelLaunchTestsP : public ::testing::Test,
                             public ::testing::WithParamInterface<
                                 std::tuple<uint32_t, uint32_t, uint32_t>> {};

TEST_P(
    zeKernelLaunchTestsP,
    GivenGlobalWorkOffsetWhenExecutingFunctionThenFunctionExecutesSuccessfully) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto context = lzt::create_context(driver);

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);
  auto module = lzt::create_module(context, device, "module_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  auto kernel = lzt::create_function(module, "module_add_constant_3");

  auto supports_global_offset = false;
  auto driver_extension_properties = lzt::get_extension_properties(driver);
  for (auto &extension : driver_extension_properties) {
    if (!std::strncmp("ZE_experimental_global_offset", extension.name,
                      ZE_MAX_EXTENSION_NAME)) {
      supports_global_offset = true;
    }
  }

  if (!supports_global_offset) {
    LOG_WARNING
        << "Driver does not support global offsets in kernel, skipping test";
    return;
  }

  auto base_size = 8;
  auto size = base_size * base_size * base_size;

  auto buffer_a = lzt::allocate_shared_memory(size, 0, 0, 0, device, context);
  auto buffer_b = lzt::allocate_device_memory(size, 0, 0, device, context);
  std::memset(buffer_a, 0, size);
  for (int x = 0; x < base_size; x++) {
    for (int y = 0; y < base_size; y++) {
      for (int z = 0; z < base_size; z++) {
        auto index = x + base_size * y + base_size * base_size * z;
        static_cast<uint8_t *>(buffer_a)[index] = (index & 0xFF);
      }
    }
  }

  const int addval = 4;

  lzt::set_argument_value(kernel, 0, sizeof(buffer_b), &buffer_b);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
  lzt::set_argument_value(kernel, 2, sizeof(base_size), &base_size);
  lzt::set_argument_value(kernel, 3, sizeof(base_size), &base_size);

  uint32_t offset_x = std::get<0>(GetParam());
  uint32_t offset_y = std::get<1>(GetParam());
  uint32_t offset_z = std::get<2>(GetParam());
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zeKernelSetGlobalOffsetExp(kernel, offset_x, offset_y, offset_z));

  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  lzt::suggest_group_size(kernel, (base_size - offset_x),
                          (base_size - offset_y), (base_size - offset_z),
                          group_size_x, group_size_y, group_size_z);
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

  ze_group_count_t group_count = {};
  group_count.groupCountX = (base_size - offset_x) / group_size_x;
  group_count.groupCountY = (base_size - offset_y) / group_size_y;
  group_count.groupCountZ = (base_size - offset_z) / group_size_z;

  LOG_DEBUG << "Offsets : x-" << offset_x << " y-" << offset_y << " z-"
            << offset_z;
  LOG_DEBUG << "Group Info:";
  LOG_DEBUG << "[X] Size: " << group_size_x
            << " Count: " << group_count.groupCountX;
  LOG_DEBUG << "[Y] Size: " << group_size_y
            << " Count: " << group_count.groupCountY;
  LOG_DEBUG << "[Z] Size: " << group_size_z
            << " Count: " << group_count.groupCountZ;

  lzt::append_memory_copy(command_list, buffer_b, buffer_a, size);
  lzt::append_barrier(command_list);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, buffer_a, buffer_b, size);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  // validation
  for (int x = 0; x < base_size; x++) {
    for (int y = 0; y < base_size; y++) {
      for (int z = 0; z < base_size; z++) {

        auto index = x + base_size * y + base_size * base_size * z;

        uint8_t val = 0;
        if (x >= offset_x && y >= offset_y && z >= offset_z) {
          val = static_cast<uint8_t>((index & 0xFF) + addval);
        } else {
          val = static_cast<uint8_t>(index & 0xFF);
        }
        ASSERT_EQ(static_cast<uint8_t *>(buffer_a)[index], val);
      }
    }
  }

  // cleanup
  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}

INSTANTIATE_TEST_CASE_P(
    KernelOffsetTests, zeKernelLaunchTestsP,
    ::testing::Combine(::testing::Values(0, 1, 4,
                                         7), // 0, 1, base_size/2, base_size-1
                       ::testing::Values(0, 1, 4, 7),
                       ::testing::Values(0, 1, 4, 7)));

class zeKernelLaunchSubDeviceTests : public zeKernelLaunchTests {
protected:
  void SetUp() override {

    auto driver = lzt::get_default_driver();
    auto devices = lzt::get_devices(driver);

    for (auto device : devices) {
      auto subdevices = lzt::get_ze_sub_devices(device);

      if (subdevices.empty()) {
        continue;
      }
      device_ = subdevices[0];
      module_ = create_module_vector(device_, "module_add");
      break;
    }
  }
};

TEST_P(
    zeKernelLaunchSubDeviceTests,
    GivenValidFunctionWhenAppendLaunchKernelOnSubDeviceThenReturnSuccessfulAndVerifyExecution) {

  if (!device_) {
    LOG_WARNING << "No sub-device for kernel execution test";
    return;
  }
  test_kernel_execution();
}

INSTANTIATE_TEST_CASE_P(
    TestFunctionAndFunctionIndirectAndMultipleFunctionsIndirect,
    zeKernelLaunchSubDeviceTests,
    testing::Values(FUNCTION, FUNCTION_INDIRECT, MULTIPLE_INDIRECT));

class ModuleGetKernelNamesTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<uint32_t> {};

TEST_P(
    ModuleGetKernelNamesTests,
    GivenValidModuleWhenGettingKernelNamesThenCorrectKernelNumberAndNamesAreReturned) {
  int num = GetParam();
  uint32_t kernel_count = 0;
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  std::string filename =
      std::to_string(num) + "kernel" + (num == 1 ? "" : "s") + ".spv";
  ze_module_handle_t module = lzt::create_module(device, filename);
  std::vector<const char *> names(num);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeModuleGetKernelNames(module, &kernel_count, nullptr));
  EXPECT_EQ(kernel_count, num);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeModuleGetKernelNames(module, &kernel_count, names.data()));

  LOG_DEBUG << kernel_count << " Kernels in Module:";
  for (uint32_t i = 0; i < kernel_count; i++) {
    LOG_DEBUG << "\t" << names[i];
    EXPECT_EQ(names[i], "kernel" + std::to_string(i + 1));
  }

  lzt::destroy_module(module);
}

INSTANTIATE_TEST_CASE_P(ModuleGetKernelNamesParamTests,
                        ModuleGetKernelNamesTests,
                        ::testing::Values(0, 1, 10, 100, 1000));
TEST(
    zeKernelSetIntermediateCacheConfigTest,
    GivenValidkernelHandleWhileSettingIntermediateCacheConfigurationThenSuccessIsReturned) {
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_module_handle_t module = lzt::create_module(
      device, "module_add.spv", ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "module_add_constant");
  lzt::set_kernel_cache_config(function, 0);
  lzt::set_kernel_cache_config(function, ZE_CACHE_CONFIG_FLAG_LARGE_SLM);
  lzt::set_kernel_cache_config(function, ZE_CACHE_CONFIG_FLAG_LARGE_DATA);
}
} // namespace
