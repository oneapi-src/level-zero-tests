/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

LZT_TEST(
    zeP2PContextCopyTests,
    GivenMultipleDevicesInDifferentContextsWhenAttemptingToUseMemoryThenMemoryIsAccessible) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  if (devices.size() < 2) {
    GTEST_SKIP() << "Not enough devices to run test";
  }

  auto device0 = devices[0];
  auto device1 = devices[1];

  // ensure devices can access each other and the appropriate memory
  auto can_access = lzt::can_access_peer(device0, device1);
  if (!can_access) {
    GTEST_SKIP() << "Devices cannot access each other";
  }

  auto dev_access_properties = lzt::get_p2p_properties(device0, device1);
  if (!(dev_access_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ACCESS)) {
    GTEST_SKIP() << "Devices cannot access each other's memory";
  }

  auto context0 = lzt::create_context(driver);
  auto context1 = lzt::create_context(driver);

  uint32_t size = 1024;
  auto verification_buffer = new uint8_t[size];

  auto buffer_0_0 = lzt::allocate_device_memory(size, 0, 0, device0, context0);
  auto buffer_0_1 = lzt::allocate_device_memory(size, 0, 0, device0, context0);

  auto buffer_1_0 = lzt::allocate_device_memory(size, 0, 0, device1, context1);
  auto buffer_1_1 = lzt::allocate_device_memory(size, 0, 0, device1, context1);

  auto command_list0 = lzt::create_command_list(context0, device0, 0);
  auto command_queue0 = lzt::create_command_queue(
      context0, device0, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  auto command_list1 = lzt::create_command_list(context1, device1, 0);
  auto command_queue1 = lzt::create_command_queue(
      context1, device1, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  uint8_t pattern = 0xAB;
  lzt::append_memory_fill(command_list0, buffer_0_0, &pattern, sizeof(pattern),
                          size, nullptr);

  lzt::append_memory_fill(command_list1, buffer_1_1, &pattern, sizeof(pattern),
                          size, nullptr);

  lzt::close_command_list(command_list0);
  lzt::close_command_list(command_list1);

  lzt::execute_command_lists(command_queue0, 1, &command_list0, nullptr);
  lzt::execute_command_lists(command_queue1, 1, &command_list1, nullptr);

  lzt::synchronize(command_queue0, UINT64_MAX);
  lzt::synchronize(command_queue1, UINT64_MAX);

  lzt::reset_command_list(command_list0);
  lzt::reset_command_list(command_list1);

  lzt::append_memory_copy(context0, command_list1, buffer_1_0, buffer_0_0, size,
                          nullptr, 0, nullptr);
  lzt::append_barrier(command_list1);
  lzt::append_memory_copy(command_list1, verification_buffer, buffer_1_0, size);
  lzt::append_barrier(command_list1);
  pattern = 0xCD;
  lzt::append_memory_fill(command_list1, buffer_1_0, &pattern, sizeof(pattern),
                          size, nullptr);

  lzt::close_command_list(command_list1);
  lzt::execute_command_lists(command_queue1, 1, &command_list1, nullptr);
  lzt::synchronize(command_queue1, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(verification_buffer[i], 0xAB);
  }

  std::memset(verification_buffer, 0, size);

  pattern = 0xAB;
  lzt::append_memory_copy(context1, command_list0, buffer_0_1, buffer_1_1, size,
                          nullptr, 0, nullptr);
  lzt::append_barrier(command_list0);
  lzt::append_memory_copy(command_list0, verification_buffer, buffer_0_1, size);
  lzt::append_barrier(command_list0);
  pattern = 0xCD;
  lzt::append_memory_fill(command_list0, buffer_0_0, &pattern, sizeof(pattern),
                          size, nullptr);
  lzt::close_command_list(command_list0);
  lzt::execute_command_lists(command_queue0, 1, &command_list0, nullptr);
  lzt::synchronize(command_queue0, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(verification_buffer[i], 0xAB);
  }

  LOG_INFO << "Starting verification of cross device cross context kernel "
              "buffer access";

  auto module0 = lzt::create_module(context0, device0, "copy_module.spv",
                                    ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  auto kernel0 = lzt::create_function(module0, "copy_data");
  int offset = 0;

  lzt::set_argument_value(kernel0, 0, sizeof(buffer_1_0), &buffer_1_0);
  lzt::set_argument_value(kernel0, 1, sizeof(buffer_1_1), &buffer_1_1);
  lzt::set_argument_value(kernel0, 2, sizeof(int), &offset);
  lzt::set_argument_value(kernel0, 3, sizeof(int), &size);

  lzt::set_group_size(kernel0, 1, 1, 1);

  ze_group_count_t group_count;
  group_count.groupCountX = 1;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::reset_command_list(command_list0);

  lzt::append_launch_function(command_list0, kernel0, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list0);
  lzt::append_memory_copy(context1, command_list0, verification_buffer,
                          buffer_1_1, size, nullptr, 0, nullptr);
  lzt::close_command_list(command_list0);

  lzt::execute_command_lists(command_queue0, 1, &command_list0, nullptr);
  lzt::synchronize(command_queue0, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(verification_buffer[i], pattern);
  }

  auto module1 = lzt::create_module(context1, device1, "copy_module.spv",
                                    ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  auto kernel1 = lzt::create_function(module1, "copy_data");

  lzt::set_argument_value(kernel1, 0, sizeof(buffer_0_0), &buffer_0_0);
  lzt::set_argument_value(kernel1, 1, sizeof(buffer_0_1), &buffer_0_1);
  lzt::set_argument_value(kernel1, 2, sizeof(int), &offset);
  lzt::set_argument_value(kernel1, 3, sizeof(int), &size);

  lzt::set_group_size(kernel1, 1, 1, 1);

  lzt::reset_command_list(command_list1);

  lzt::append_launch_function(command_list1, kernel1, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list1);
  lzt::append_memory_copy(context0, command_list1, verification_buffer,
                          buffer_0_1, size, nullptr, 0, nullptr);

  lzt::close_command_list(command_list1);
  lzt::execute_command_lists(command_queue1, 1, &command_list1, nullptr);
  lzt::synchronize(command_queue1, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(verification_buffer[i], pattern);
  }

  // cleanup
  delete[] verification_buffer;
  lzt::free_memory(context0, buffer_0_0);
  lzt::free_memory(context0, buffer_0_1);
  lzt::free_memory(context1, buffer_1_0);
  lzt::free_memory(context1, buffer_1_1);
  lzt::destroy_function(kernel0);
  lzt::destroy_function(kernel1);
  lzt::destroy_module(module0);
  lzt::destroy_module(module1);
  lzt::destroy_command_list(command_list0);
  lzt::destroy_command_list(command_list1);
  lzt::destroy_command_queue(command_queue0);
  lzt::destroy_command_queue(command_queue1);
  lzt::destroy_context(context0);
  lzt::destroy_context(context1);
}

} // namespace
