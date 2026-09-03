/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "vulkan_interop/vulkan_library.hpp"
#include "vulkan_interop/vulkan_interop_helper.hpp"

#include "test_harness/test_harness.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstring>
#include <vector>

struct VulkanInteroperabilityTests : public ::testing::Test {
  void SetUp() override {
    if (!vlk::initialize_loader()) {
      GTEST_SKIP() << "Cannot load Vulkan library - skipping tests.";
    }

    vk_instance = vlk::create_instance();

    auto vk_physical_devices = vlk::get_physical_devices(vk_instance);

    l0_device = lzt::zeDevice::get_instance()->get_device();
    ze_device_properties_t l0_device_props =
        lzt::get_device_properties(l0_device);

    bool devices_matched = false;
    for (auto &vk_pdev : vk_physical_devices) {
      VkPhysicalDeviceProperties vk_device_props = {};
      vkGetPhysicalDeviceProperties(vk_pdev, &vk_device_props);

      if (vk_device_props.deviceID == l0_device_props.deviceId &&
          vk_device_props.apiVersion >= vlk::REQUIRED_API_VERSION) {
        devices_matched = true;
        vk_physical_device = vk_pdev;
        break;
      }
    }

    if (!devices_matched) {
      GTEST_SKIP() << "Device doesn't support Vulkan.";
    }

    std::vector<const char *> required_device_extensions = {
        vlk::get_platform_external_memory_extension_name(),
        vlk::get_platform_external_semaphore_extension_name()};
    auto vk_device_extension_props =
        vlk::get_device_extension_properties(vk_physical_device);

    const bool has_required_extensions = std::all_of(
        std::begin(required_device_extensions),
        std::end(required_device_extensions), [&](const char *req_extension) {
          auto it = std::find_if(std::begin(vk_device_extension_props),
                                 std::end(vk_device_extension_props),
                                 [&](const VkExtensionProperties &ext_prop) {
                                   return strcmp(ext_prop.extensionName,
                                                 req_extension) == 0;
                                 });
          return (it != std::end(vk_device_extension_props));
        });

    if (!has_required_extensions) {
      GTEST_SKIP() << "Device doesn't support required Vulkan extensions.";
    }

    if (!vlk::has_timeline_semaphore_support(vk_physical_device)) {
      GTEST_SKIP() << "Device doesn't support Vulkan timeline semaphores.";
    }

    vk_queue_family_index = vlk::get_queue_family_index(vk_physical_device);
    vk_device = vlk::create_device(vk_physical_device, vk_queue_family_index);
  }

  void TearDown() override {
    vlk::destroy_device(vk_device);
    vlk::destroy_instance(vk_instance);
  }

  void semaphore_import_test(VkSemaphoreType vk_type) {
    VkSemaphore vk_semaphore = vlk::create_export_semaphore(vk_device, vk_type);

    auto external_semaphore_handle =
        import_semaphore(l0_device, vk_device, vk_semaphore, vk_type);
    EXPECT_NE(external_semaphore_handle, nullptr);

    lzt::release_external_semaphore(external_semaphore_handle);
    vlk::destroy_semaphore(vk_device, vk_semaphore);
  }

  VkInstance vk_instance = VK_NULL_HANDLE;
  VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;
  VkDevice vk_device = VK_NULL_HANDLE;
  uint32_t vk_queue_family_index = 0;

  ze_device_handle_t l0_device;
};
