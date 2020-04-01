/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_MODULE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_MODULE_HPP

#include <level_zero/ze_api.h>
#include <string>

namespace level_zero_tests {

ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename);
ze_module_handle_t create_module(ze_device_handle_t device,
                                 const std::string filename,
                                 const ze_module_format_t format,
                                 const char *build_flags,
                                 ze_module_build_log_handle_t *phBuildLog);
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
                                   ze_kernel_flag_t flag,
                                   std::string func_name);
void destroy_function(ze_kernel_handle_t function);

// This function is useful when only a single argument is needed.
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, int group_size,
                                 void *arg);

struct FunctionArg {
  size_t arg_size;
  void *arg_value;
};

// Group size can only be set in x dimension
// Accepts arbitrary amounts of function arguments
void create_and_execute_function(ze_device_handle_t device,
                                 ze_module_handle_t module,
                                 std::string func_name, int group_size,
                                 const std::vector<FunctionArg> &args);

uint32_t get_kernel_source_attribute_size(ze_kernel_handle_t hKernel);
void get_kernel_attribute(ze_kernel_handle_t hKernel,
                          ze_kernel_attribute_t attr, uint32_t *size,
                          void *pValue);
void set_kernel_attribute(ze_kernel_handle_t hKernel,
                          ze_kernel_attribute_t attr, uint32_t size,
                          void *pValue);
} // namespace level_zero_tests

#endif
