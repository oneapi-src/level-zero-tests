/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "gtest/gtest.h"
#include <level_zero/ze_api.h>
#include <thread>
#include <chrono>
#include <string>

namespace level_zero_tests {

#ifdef ZE_MODULE_PROGRAM_EXP_NAME
ze_module_handle_t
create_program_module(ze_context_handle_t context, ze_device_handle_t device,
                      std::vector<module_program_modules_t> modules_in) {

  ze_module_program_exp_desc_t module_program_desc = {};
  module_program_desc.stype = ZE_STRUCTURE_TYPE_MODULE_PROGRAM_EXP_DESC;
  module_program_desc.count = modules_in.size();

  std::vector<size_t> input_sizes;
  std::vector<char *> build_flags;
  std::vector<ze_module_constants_t *> constants;
  std::vector<uint8_t *> module_data;

  for (auto module : modules_in) {
    const std::vector<uint8_t> binary_file =
        level_zero_tests::load_binary_file(module.filename);

    auto binary_data = new uint8_t[binary_file.size()];
    memcpy(binary_data, binary_file.data(), binary_file.size());

    module_data.push_back(binary_data);
    input_sizes.push_back(size_t(binary_file.size()));
    build_flags.push_back(module.pBuildFlags);
    constants.push_back(module.pConstants);
  }

  module_program_desc.inputSizes = input_sizes.data();
  module_program_desc.pInputModules = (const uint8_t **)module_data.data();
  module_program_desc.pBuildFlags = (const char **)build_flags.data();
  module_program_desc.pConstants =
      (const ze_module_constants_t **)constants.data();

  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  module_description.pNext = &module_program_desc;
  module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;

  auto device_initial = device;
  auto context_initial = context;

  ze_module_handle_t module;
  EXPECT_ZE_RESULT_SUCCESS(
      zeModuleCreate(context, device, &module_description, &module, nullptr));
  EXPECT_EQ(context, context_initial);
  EXPECT_EQ(device, device_initial);

  for (auto data : module_data)
    delete[] data;

  return module;
}

#endif

ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename) {

  return (
      create_module(device, filename, ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr));
}

ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *p_build_log) {

  return create_module(lzt::get_default_context(), device, filename, format,
                       build_flags, p_build_log);
}

ze_module_handle_t create_module(ze_context_handle_t context,
                                 ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *p_build_log) {
  ze_result_t build_result = ZE_RESULT_SUCCESS;
  return create_module(context, device, filename, format, build_flags,
                       p_build_log, &build_result);
}

ze_module_handle_t create_module(ze_context_handle_t context,
                                 ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *p_build_log,
                                 ze_result_t *build_result) {

  ze_module_desc_t module_description = {};
  module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
  ze_module_handle_t module;
  ze_module_constants_t module_constants = {};
  const std::vector<uint8_t> binary_file =
      level_zero_tests::load_binary_file(filename);

  module_description.pNext = nullptr;
  module_description.format = format;
  module_description.inputSize = static_cast<uint32_t>(binary_file.size());
  module_description.pInputModule = binary_file.data();
  module_description.pBuildFlags = build_flags;
  module_description.pConstants = &module_constants;

  auto context_initial = context;
  auto device_initial = device;
  *build_result = zeModuleCreate(context, device, &module_description, &module,
                                 p_build_log);
  EXPECT_ZE_RESULT_SUCCESS(*build_result);
  EXPECT_EQ(context, context_initial);
  EXPECT_EQ(device, device_initial);

  return module;
}

size_t get_build_log_size(ze_module_build_log_handle_t build_log) {
  size_t build_log_size = 0;
  auto build_log_initial = build_log;
  EXPECT_ZE_RESULT_SUCCESS(
      zeModuleBuildLogGetString(build_log, &build_log_size, nullptr));
  EXPECT_EQ(build_log, build_log_initial);
  EXPECT_GT(build_log_size, 0);
  return build_log_size;
}

std::string get_build_log_string(ze_module_build_log_handle_t build_log) {
  size_t build_log_size = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zeModuleBuildLogGetString(build_log, &build_log_size, nullptr));

  EXPECT_GT(build_log_size, 0);

  std::vector<char> build_log_c_string(build_log_size);
  EXPECT_ZE_RESULT_SUCCESS(zeModuleBuildLogGetString(
      build_log, &build_log_size, build_log_c_string.data()));
  return std::string(build_log_c_string.begin(), build_log_c_string.end());
}

void dynamic_link(uint32_t num_modules, ze_module_handle_t *modules) {
  dynamic_link(num_modules, modules, nullptr);
}

void dynamic_link(uint32_t num_modules, ze_module_handle_t *modules,
                  ze_module_build_log_handle_t *link_log) {
  ze_result_t result = ZE_RESULT_SUCCESS;
  dynamic_link(num_modules, modules, link_log, &result);
}

void dynamic_link(uint32_t num_modules, ze_module_handle_t *modules,
                  ze_module_build_log_handle_t *link_log, ze_result_t *result) {
  std::vector<ze_module_handle_t> modules_initial(num_modules);
  memcpy(modules_initial.data(), modules,
         sizeof(ze_module_handle_t) * num_modules);
  *result = zeModuleDynamicLink(num_modules, modules, link_log);
  EXPECT_ZE_RESULT_SUCCESS(*result);
  for (int i = 0; i < num_modules; i++) {
    EXPECT_EQ(modules[i], modules_initial[i]);
  }
}

size_t get_native_binary_size(ze_module_handle_t module) {
  size_t native_binary_size = 0;
  auto module_initial = module;
  EXPECT_ZE_RESULT_SUCCESS(
      zeModuleGetNativeBinary(module, &native_binary_size, nullptr));
  EXPECT_EQ(module, module_initial);
  EXPECT_GT(native_binary_size, 0);
  return native_binary_size;
}

void save_native_binary_file(ze_module_handle_t module,
                             const std::string filename) {
  size_t native_binary_size = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zeModuleGetNativeBinary(module, &native_binary_size, nullptr));
  EXPECT_GT(native_binary_size, 0);

  std::vector<uint8_t> native_binary(native_binary_size);
  EXPECT_ZE_RESULT_SUCCESS(zeModuleGetNativeBinary(module, &native_binary_size,
                                                   native_binary.data()));
  level_zero_tests::save_binary_file(native_binary, filename);
}

void destroy_build_log(ze_module_build_log_handle_t build_log) {
  EXPECT_ZE_RESULT_SUCCESS(zeModuleBuildLogDestroy(build_log));
}

void set_argument_value(ze_kernel_handle_t hFunction, uint32_t argIndex,
                        size_t argSize, const void *pArgValue) {
  auto function_initial = hFunction;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelSetArgumentValue(hFunction, argIndex, argSize, pArgValue));
  EXPECT_EQ(hFunction, function_initial);
}

void set_group_size(ze_kernel_handle_t hFunction, uint32_t groupSizeX,
                    uint32_t groupSizeY, uint32_t groupSizeZ) {
  auto function_initial = hFunction;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelSetGroupSize(hFunction, groupSizeX, groupSizeY, groupSizeZ));
  EXPECT_EQ(hFunction, function_initial);
}

void suggest_group_size(ze_kernel_handle_t hFunction, uint32_t globalSizeX,
                        uint32_t globalSizeY, uint32_t globalSizeZ,
                        uint32_t &groupSizeX, uint32_t &groupSizeY,
                        uint32_t &groupSizeZ) {
  auto function_initial = hFunction;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelSuggestGroupSize(hFunction, globalSizeX, globalSizeY, globalSizeZ,
                               &groupSizeX, &groupSizeY, &groupSizeZ));
  EXPECT_EQ(hFunction, function_initial);
}

void destroy_module(ze_module_handle_t module) {
  EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
}

ze_kernel_handle_t create_function(ze_module_handle_t module,
                                   std::string func_name) {
  return create_function(module, 0, func_name);
}

ze_kernel_handle_t create_function(ze_module_handle_t module,
                                   ze_kernel_flags_t flag,
                                   std::string func_name) {
  ze_kernel_handle_t kernel;
  ze_kernel_desc_t kernel_description = {};
  kernel_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;

  kernel_description.pNext = nullptr;
  kernel_description.flags = flag;
  kernel_description.pKernelName = func_name.c_str();
  auto module_initial = module;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelCreate(module, &kernel_description, &kernel));
  EXPECT_EQ(module, module_initial);
  return kernel;
}

void destroy_function(ze_kernel_handle_t kernel) {

  EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
}

ze_kernel_properties_t get_kernel_properties(ze_kernel_handle_t kernel) {
  ze_kernel_properties_t properties{};
  properties.stype = ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES;
  properties.pNext = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelGetProperties(kernel, &properties));
  return properties;
}

// Currently limited to creating function with 1d group and single argument.
// Expand as needed.
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, int group_size,
                                 void *arg, bool is_immediate) {
  std::vector<FunctionArg> args;
  if (arg != nullptr) {
    FunctionArg func_arg{sizeof(arg), &arg};
    args.push_back(func_arg);
  }
  create_and_execute_function(device, module, func_name, group_size, args,
                              is_immediate);
}

void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, int group_size,
                                 const std::vector<FunctionArg> &args,
                                 bool is_immediate) {

  ze_kernel_handle_t function = create_function(module, func_name);
  zeCommandBundle cmd_bundle = create_command_bundle(device, is_immediate);
  uint32_t group_size_x = group_size;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(
      function, group_size, 1, 1, &group_size_x, &group_size_y, &group_size_z));

  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));

  ze_kernel_properties_t function_properties = get_kernel_properties(function);
  EXPECT_EQ(function_properties.numKernelArgs, args.size());

  int i = 0;
  for (auto arg : args) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(function, i++, arg.arg_size, arg.arg_value));
  }

  ze_group_count_t thread_group_dimensions;
  thread_group_dimensions.groupCountX = 1;
  thread_group_dimensions.groupCountY = 1;
  thread_group_dimensions.groupCountZ = 1;

  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
      cmd_bundle.list, function, &thread_group_dimensions, nullptr, 0,
      nullptr));

  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendBarrier(cmd_bundle.list, nullptr, 0, nullptr));
  if (is_immediate) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandListHostSynchronize(cmd_bundle.list, UINT64_MAX));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListClose(cmd_bundle.list));

    EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(
        cmd_bundle.queue, 1, &cmd_bundle.list, nullptr));

    EXPECT_ZE_RESULT_SUCCESS(
        zeCommandQueueSynchronize(cmd_bundle.queue, UINT64_MAX));
  }

  destroy_function(function);
  destroy_command_bundle(cmd_bundle);
}

void kernel_set_indirect_access(ze_kernel_handle_t hKernel,
                                ze_kernel_indirect_access_flags_t flags) {
  auto kernel_initial = hKernel;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelSetIndirectAccess(hKernel, flags));
  EXPECT_EQ(hKernel, kernel_initial);
}

void kernel_get_indirect_access(ze_kernel_handle_t hKernel,
                                ze_kernel_indirect_access_flags_t *flags) {
  auto kernel_initial = hKernel;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelGetIndirectAccess(hKernel, flags));
  EXPECT_EQ(hKernel, kernel_initial);
}

std::string kernel_get_name_string(ze_kernel_handle_t hKernel) {
  size_t kernel_name_size = 0;
  auto kernel_initial = hKernel;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelGetName(hKernel, &kernel_name_size, nullptr));

  char *kernel_name = new char[kernel_name_size];
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelGetName(hKernel, &kernel_name_size, kernel_name));
  EXPECT_EQ(hKernel, kernel_initial);

  std::string kernel_name_str(kernel_name, kernel_name_size - 1);
  delete[] kernel_name;
  return kernel_name_str;
}

std::string kernel_get_source_attribute(ze_kernel_handle_t hKernel) {
  uint32_t size = 0;
  auto kernel_initial = hKernel;
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelGetSourceAttributes(hKernel, &size, nullptr));

  char *source_string = new char[size];
  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelGetSourceAttributes(hKernel, &size, &source_string));
  EXPECT_EQ(hKernel, kernel_initial);

  std::string source_string_str(source_string, size - 1);
  delete[] source_string;
  return source_string_str;
}

#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
void set_kernel_scheduling_hint(ze_kernel_handle_t kernel,
                                ze_scheduling_hint_exp_flags_t hints) {
  ze_scheduling_hint_exp_desc_t desc = {};
  desc.flags = hints;
  desc.stype = ZE_STRUCTURE_TYPE_SCHEDULING_HINT_EXP_DESC;
  auto kernel_initial = kernel;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelSchedulingHintExp(kernel, &desc));
  EXPECT_EQ(kernel, kernel_initial);
}
#endif

} // namespace level_zero_tests
