/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"

namespace level_zero_tests {

void *ocl_register_memory(cl_context context, cl_mem mem) {
  void *ptr;
  EXPECT_ZE_RESULT_SUCCESS(zeDeviceRegisterCLMemory(
      lzt::zeDevice::get_instance()->get_device(), context, mem, &ptr));
  return ptr;
}

ze_command_queue_handle_t
ocl_register_commandqueue(cl_context context, cl_command_queue command_queue) {

  ze_command_queue_handle_t l0_command_queue;
  EXPECT_ZE_RESULT_SUCCESS(zeDeviceRegisterCLCommandQueue(
      lzt::zeDevice::get_instance()->get_device(), context, command_queue,
      &l0_command_queue));
  return l0_command_queue;
}

ze_module_handle_t ocl_register_program(cl_context context,
                                        cl_program program) {

  ze_module_handle_t module_handle = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(
      zeDeviceRegisterCLProgram(lzt::zeDevice::get_instance()->get_device(),
                                context, program, &module_handle));
  return module_handle;
}

}; // namespace level_zero_tests
