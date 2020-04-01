/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_XE_TEST_HARNESS_OCL_HPP
#define level_zero_tests_XE_TEST_HARNESS_OCL_HPP

#include <level_zero/ze_api.h>

namespace level_zero_tests {

void *ocl_register_memory(cl_context context, cl_mem mem);

ze_command_queue_handle_t
ocl_register_commandqueue(cl_context context, cl_command_queue command_queue);

ze_module_handle_t ocl_register_program(cl_context context, cl_program program);

}; // namespace level_zero_tests

#endif
