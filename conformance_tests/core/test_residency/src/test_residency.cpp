/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeContextMakeResidentTests : public ::testing::Test {
protected:
  void SetUp() override { memory_ = lzt::allocate_device_memory(size_); }

  void TearDown() override {
    EXPECT_ZE_RESULT_SUCCESS(zeMemFree(lzt::get_default_context(), memory_));
  }

  void *memory_ = nullptr;
  const size_t size_ = 1024;
};

LZT_TEST_F(zeContextMakeResidentTests,
           GivenDeviceMemoryWhenMakingMemoryResidentThenSuccessIsReturned) {
  EXPECT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      memory_, size_));
}

LZT_TEST(
    zeContextMakeResidentMultiDeviceTests,
    GivenDeviceMemoryWhenMakingMemoryResidentOnMultipleDevicesWithP2PSupportThenSuccessIsReturned) {
  auto devices = lzt::get_ze_devices(lzt::get_default_driver());
  if (devices.size() < 2) {
    LOG_INFO << "Test cannot be run with less than 2 Devices";
    GTEST_SKIP();
  }
  size_t size = 1024;
  for (uint32_t i = 0; i < devices.size(); i++) {
    for (uint32_t j = 0; j < devices.size(); j++) {
      if (i == j) {
        continue;
      }
      if (!lzt::can_access_peer(devices[i], devices[j])) {
        LOG_INFO << "FAILURE:  Device-to-Device access disabled";
        GTEST_SKIP();
      } else if (!lzt::can_access_peer(devices[j], devices[i])) {
        LOG_INFO << "FAILURE:  Device-to-Device access disabled";
        GTEST_SKIP();
      } else {
        void *memory = lzt::allocate_device_memory(size, 1, 0, devices[j],
                                                   lzt::get_default_context());
        EXPECT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(
            lzt::get_default_context(), devices[i], memory, size));
        lzt::free_memory(memory);
        memory = lzt::allocate_device_memory(size, 1, 0, devices[i],
                                             lzt::get_default_context());
        EXPECT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(
            lzt::get_default_context(), devices[j], memory, size));
        lzt::free_memory(memory);
      }
    }
  }
}

typedef struct _node {
  uint32_t value;
  struct _node *next;
} node;

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedMemoryWhenMakingMemoryResidentUsingAPIThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const uint32_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      GTEST_SKIP();
    }

    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, "residency_function");

    uint32_t device_flags = 0U;
    uint32_t host_flags = 0U;

    node *data = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    data->value = 0;
    node *temp = data;
    for (uint32_t i = 0U; i < size; i++) {
      temp->next = static_cast<node *>(lzt::allocate_shared_memory(
          sizeof(node), 1, device_flags, host_flags, device, context));
      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(uint32_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    temp = data->next;
    node *temp2;
    for (uint32_t i = 0U; i < size; i++) {
      temp2 = temp->next;
      lzt::make_memory_resident(device, temp, sizeof(node));
      temp = temp2;
    }

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    temp = data->next;
    for (uint32_t i = 0U; i < size; i++) {
      lzt::evict_memory(device, temp, sizeof(node));
      temp = temp->next;
    }

    // cleanup
    temp = data;
    // total of size elements linked *after* initial element
    for (uint32_t i = 0U; i < size + 1; i++) {
      // the kernel increments each node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      lzt::free_memory(context, temp);
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
}

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedSystemMemoryWhenMakingMemoryResidentUsingAPIThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const uint32_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      GTEST_SKIP();
    }

    if (!lzt::supports_shared_system_alloc(device)) {
      FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
                "system memory";
    }

    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, "residency_function");

    auto device_flags = 0;
    auto host_flags = 0;

    node *data = new node;
    data->value = 0;
    node *temp = data;

    for (uint32_t i = 0U; i < size; i++) {
      temp->next = new node;

      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(uint32_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    temp = data->next;
    node *temp2;
    for (uint32_t i = 0U; i < size; i++) {
      temp2 = temp->next;
      lzt::make_memory_resident(device, temp, sizeof(node));
      temp = temp2;
    }
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    temp = data->next;
    for (uint32_t i = 0U; i < size; i++) {
      lzt::evict_memory(device, temp, sizeof(node));
      temp = temp->next;
    }

    // cleanup
    temp = data;
    // total of size elements linked *after* initial element
    for (uint32_t i = 0U; i < size + 1; i++) {
      // kernel should increment node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      delete temp;
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
}

void RunGivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagTest(
    bool is_immediate) {
  const uint32_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  auto context = lzt::get_default_context();

  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      GTEST_SKIP();
    }

    // set up
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, ZE_KERNEL_FLAG_FORCE_RESIDENCY,
                                  "residency_function");

    uint32_t device_flags = 0U;
    uint32_t host_flags = 0U;

    node *data = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    data->value = 0;
    node *temp = data;
    for (uint32_t i = 0U; i < size; i++) {
      temp->next = static_cast<node *>(lzt::allocate_shared_memory(
          sizeof(node), 1, device_flags, host_flags, device, context));
      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::kernel_set_indirect_access(kernel,
                                    ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED);

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(uint32_t), &size);
    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    // cleanup
    temp = data;
    node *temp2;
    for (uint32_t i = 0U; i < size + 1; i++) {
      // kernel should increment each node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      lzt::free_memory(context, temp);
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_bundle(cmd_bundle);
  }
}

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagThenMemoryIsMadeResidentAndUpdatedCorrectly) {
  RunGivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagTest(false);
}

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagOnImmediateCmdListThenMemoryIsMadeResidentAndUpdatedCorrectly) {
  RunGivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagTest(true);
}

void RunGivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagTest(
    bool is_immediate) {
  const uint32_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      GTEST_SKIP();
    }

    if (!lzt::supports_shared_system_alloc(device)) {
      FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
                "system memory";
    }
    // set up
    auto cmd_bundle = lzt::create_command_bundle(device, is_immediate);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, ZE_KERNEL_FLAG_FORCE_RESIDENCY,
                                  "residency_function");

    lzt::kernel_set_indirect_access(kernel,
                                    ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED);

    auto device_flags = 0;
    auto host_flags = 0;

    node *data = new node;
    data->value = 0;
    node *temp = data;
    for (uint32_t i = 0U; i < size; i++) {
      temp->next = new node;

      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(uint32_t), &size);
    lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr,
                                0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    // cleanup
    temp = data;
    node *temp2;
    // total of size elements linked *after* initial element
    for (uint32_t i = 0U; i < size + 1; i++) {
      // kernel should increment node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      delete temp;
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_bundle(cmd_bundle);
  }
}

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagThenMemoryIsMadeResidentAndUpdatedCorrectly) {
  RunGivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagTest(false);
}

LZT_TEST_F(
    zeContextMakeResidentTests,
    GivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagOnImmediateCmdListThenMemoryIsMadeResidentAndUpdatedCorrectly) {
  RunGivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagTest(true);
}

class zeContextEvictMemoryTests : public zeContextMakeResidentTests {};

LZT_TEST_F(
    zeContextEvictMemoryTests,
    GivenResidentDeviceMemoryWhenEvictingResidentMemoryThenSuccessIsReturned) {
  EXPECT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      memory_, size_));
  EXPECT_ZE_RESULT_SUCCESS(zeContextEvictMemory(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      memory_, size_));
}

class zeDeviceMakeImageResidentTests : public testing::Test {};

LZT_TEST_F(zeDeviceMakeImageResidentTests,
           GivenDeviceImageWhenMakingImageResidentThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  lzt::zeImageCreateCommon img;
  EXPECT_ZE_RESULT_SUCCESS(zeContextMakeImageResident(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      img.dflt_device_image_));
}

class zeContextEvictImageTests : public zeDeviceMakeImageResidentTests {};

LZT_TEST_F(
    zeContextEvictImageTests,
    GivenResidentDeviceImageWhenEvictingResidentImageThenSuccessIsReturned) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  lzt::zeImageCreateCommon img;
  EXPECT_ZE_RESULT_SUCCESS(zeContextMakeImageResident(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      img.dflt_device_image_));
  EXPECT_ZE_RESULT_SUCCESS(zeContextEvictImage(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      img.dflt_device_image_));
}

} // namespace
