/*
 *
 * Copyright (C) 2019 Intel Corporation
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
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemFree(lzt::get_default_context(), memory_));
  }

  void *memory_ = nullptr;
  const size_t size_ = 1024;
};

TEST_F(zeContextMakeResidentTests,
       GivenDeviceMemoryWhenMakingMemoryResidentThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextMakeMemoryResident(
                lzt::get_default_context(),
                lzt::zeDevice::get_instance()->get_device(), memory_, size_));
}

typedef struct _node {
  uint32_t value;
  struct _node *next;
} node;

TEST_F(
    zeContextMakeResidentTests,
    GivenSharedMemoryWhenMakingMemoryResidentUsingAPIThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const size_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      return;
    }

    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, "residency_function");

    auto device_flags = 0;
    auto host_flags = 0;

    node *data = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    data->value = 0;
    node *temp = data;
    for (int i = 0; i < size; i++) {
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
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    temp = data->next;
    node *temp2;
    for (int i = 0; i < size; i++) {
      temp2 = temp->next;
      lzt::make_memory_resident(device, temp, sizeof(node));
      temp = temp2;
    }

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    temp = data->next;
    for (int i = 0; i < size; i++) {
      lzt::evict_memory(device, temp, sizeof(node));
      temp = temp->next;
    }

    // cleanup
    temp = data;
    // total of size elements linked *after* initial element
    for (int i = 0; i < size + 1; i++) {
      // the kernel increments each node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      lzt::free_memory(temp);
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
}

TEST_F(
    zeContextMakeResidentTests,
    GivenSharedSystemMemoryWhenMakingMemoryResidentUsingAPIThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const size_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      return;
    }

    auto mem_access_props = lzt::get_memory_access_properties(device);
    auto systemMemSupported = mem_access_props.sharedSystemAllocCapabilities &
                              ZE_MEMORY_ACCESS_CAP_FLAG_RW;
    if (!systemMemSupported) {
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

    for (int i = 0; i < size; i++) {
      temp->next = new node;

      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    temp = data->next;
    node *temp2;
    for (int i = 0; i < size; i++) {
      temp2 = temp->next;
      lzt::make_memory_resident(device, temp, sizeof(node));
      temp = temp2;
    }
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    temp = data->next;
    for (int i = 0; i < size; i++) {
      lzt::evict_memory(device, temp, sizeof(node));
      temp = temp->next;
    }

    // cleanup
    temp = data;
    // total of size elements linked *after* initial element
    for (int i = 0; i < size + 1; i++) {
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

TEST_F(
    zeContextMakeResidentTests,
    GivenSharedMemoryWhenMakingMemoryResidentUsingKernelFlagThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const size_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  auto context = lzt::get_default_context();

  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      return;
    }

    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);

    module = lzt::create_module(device, "residency_tests.spv");
    kernel = lzt::create_function(module, ZE_KERNEL_FLAG_FORCE_RESIDENCY,
                                  "residency_function");

    auto device_flags = 0;
    auto host_flags = 0;

    node *data = static_cast<node *>(lzt::allocate_shared_memory(
        sizeof(node), 1, device_flags, host_flags, device, context));
    data->value = 0;
    node *temp = data;
    for (int i = 0; i < size; i++) {
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
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    // cleanup
    temp = data;
    node *temp2;
    for (int i = 0; i < size + 1; i++) {
      // kernel should increment each node's value by 1
      ASSERT_EQ(temp->value, i + 1);

      temp2 = temp->next;
      lzt::free_memory(temp);
      temp = temp2;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::destroy_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
  }
}

TEST_F(
    zeContextMakeResidentTests,
    GivenSharedSystemMemoryWhenMakingMemoryResidentUsingKernelFlagThenMemoryIsMadeResidentAndUpdatedCorrectly) {

  const size_t size = 5;
  ze_module_handle_t module;
  ze_kernel_handle_t kernel;

  auto driver = lzt::get_default_driver();
  for (auto device : lzt::get_devices(driver)) {
    auto properties = lzt::get_device_properties(device);

    if (properties.flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      LOG_INFO << "[" << properties.name << "] "
               << "Device has on demand page fault support - skipping";
      return;
    }

    auto mem_access_props = lzt::get_memory_access_properties(device);
    auto systemMemSupported = mem_access_props.sharedSystemAllocCapabilities &
                              ZE_MEMORY_ACCESS_CAP_FLAG_RW;
    if (!systemMemSupported) {
      FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
                "system memory";
    }
    // set up
    auto command_list = lzt::create_command_list(device);
    auto command_queue = lzt::create_command_queue(device);

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
    for (int i = 0; i < size; i++) {
      temp->next = new node;

      temp = temp->next;
      temp->value = i + 1;
    }

    ze_group_count_t group_count;
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;

    lzt::set_argument_value(kernel, 0, sizeof(node *), &data);
    lzt::set_argument_value(kernel, 1, sizeof(size_t), &size);
    lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);

    // cleanup
    temp = data;
    node *temp2;
    // total of size elements linked *after* initial element
    for (int i = 0; i < size + 1; i++) {
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

class zeContextEvictMemoryTests : public zeContextMakeResidentTests {};

TEST_F(
    zeContextEvictMemoryTests,
    GivenResidentDeviceMemoryWhenEvictingResidentMemoryThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextMakeMemoryResident(
                lzt::get_default_context(),
                lzt::zeDevice::get_instance()->get_device(), memory_, size_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextEvictMemory(lzt::get_default_context(),
                                 lzt::zeDevice::get_instance()->get_device(),
                                 memory_, size_));
}

class zeDeviceMakeImageResidentTests : public testing::Test,
                                       public lzt::zeImageCreateCommon {};

TEST_F(zeDeviceMakeImageResidentTests,
       GivenDeviceImageWhenMakingImageResidentThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeContextMakeImageResident(
                                   lzt::get_default_context(),
                                   lzt::zeDevice::get_instance()->get_device(),
                                   dflt_device_image_));
}

class zeContextEvictImageTests : public zeDeviceMakeImageResidentTests {};

TEST_F(zeContextEvictImageTests,
       GivenResidentDeviceImageWhenEvictingResidentImageThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeContextMakeImageResident(
                                   lzt::get_default_context(),
                                   lzt::zeDevice::get_instance()->get_device(),
                                   dflt_device_image_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextEvictImage(lzt::get_default_context(),
                                lzt::zeDevice::get_instance()->get_device(),
                                dflt_device_image_));
}

} // namespace
