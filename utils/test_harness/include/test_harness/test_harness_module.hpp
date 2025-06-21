/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_MODULE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_MODULE_HPP

#include <level_zero/ze_api.h>
#include <string>
#include <vector>

namespace level_zero_tests {

#ifdef ZE_MODULE_PROGRAM_EXP_NAME
struct module_program_modules_t {
  std::string filename;
  char *pBuildFlags;
  ze_module_constants_t *pConstants;
};

ze_module_handle_t
create_program_module(ze_context_handle_t context, ze_device_handle_t device,
                      std::vector<module_program_modules_t> modules_in);
#endif

ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename);
ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *phBuildLog);
ze_module_handle_t create_module(ze_context_handle_t context,
                                 ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *p_build_log);
ze_module_handle_t create_module(ze_context_handle_t context,
                                 ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *p_build_log,
                                 ze_result_t *build_result);
void destroy_module(ze_module_handle_t module);
size_t get_build_log_size(const ze_module_build_log_handle_t build_log);
std::string get_build_log_string(const ze_module_build_log_handle_t build_log);
size_t get_native_binary_size(const ze_module_handle_t module);
void save_native_binary_file(const ze_module_handle_t module,
                             const std::string filename);
void destroy_build_log(const ze_module_build_log_handle_t build_log);
void set_argument_value(ze_kernel_handle_t hFunction, uint32_t argIndex,
                        size_t argSize, const void *pArgValue);
void suggest_group_size(ze_kernel_handle_t hFunction, uint32_t globalSizeX,
                        uint32_t globalSizeY, uint32_t globalSizeZ,
                        uint32_t &groupSizeX, uint32_t &groupSizeY,
                        uint32_t &groupSizeZ);
void set_group_size(ze_kernel_handle_t hFunction, uint32_t groupSizeX,
                    uint32_t groupSizeY, uint32_t groupSizeZ);
ze_kernel_handle_t create_function(ze_module_handle_t module,
                                   std::string func_name);
ze_kernel_handle_t create_function(ze_module_handle_t module,
                                   ze_kernel_flags_t flag,
                                   std::string func_name);
void destroy_function(ze_kernel_handle_t function);
ze_kernel_properties_t get_kernel_properties(ze_kernel_handle_t kernel);

// This function is useful when only a single argument is needed.
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size_x,
                                 void *arg, bool is_immediate);
  void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size_x,
                                 uint32_t group_size_y, void *arg,
                                 bool is_immediate);                             
void kernel_set_indirect_access(ze_kernel_handle_t hKernel,
                                ze_kernel_indirect_access_flags_t flags);
void kernel_get_indirect_access(ze_kernel_handle_t hKernel,
                                ze_kernel_indirect_access_flags_t *flags);
std::string kernel_get_name_string(ze_kernel_handle_t hKernel);
std::string kernel_get_source_attribute(ze_kernel_handle_t hKernel);

struct FunctionArg {
  size_t arg_size;
  void *arg_value;
};

// Group size can only be set in x dimension
// Accepts arbitrary amounts of function arguments
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size_x,
                                 const std::vector<FunctionArg> &args,
                                 bool is_immediate);

void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, uint32_t group_size_x,
                                 uint32_t group_size_y,
                                 const std::vector<FunctionArg> &args,
                                 bool is_immediate);

#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
void set_kernel_scheduling_hint(ze_kernel_handle_t kernel,
                                ze_scheduling_hint_exp_flags_t hints);
#endif

} // namespace level_zero_tests

#endif
