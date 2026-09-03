/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#define VK_CHECK(CALL)                                                         \
  do {                                                                         \
    if (VkResult res = (CALL); res != VK_SUCCESS) {                            \
      throw std::runtime_error(std::string(#CALL " failed with: ") +           \
                               std::to_string(res));                           \
    }                                                                          \
  } while (false)

inline PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
inline PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = nullptr;

#ifdef __linux__
inline PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR = nullptr;
#else
inline PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR = nullptr;
#endif

#define FOR_VULKAN_GLOBAL_FUNCTIONS(DO)                                        \
  DO(vkEnumerateInstanceExtensionProperties)                                   \
  DO(vkEnumerateInstanceLayerProperties)                                       \
  DO(vkCreateInstance)

#define FOR_VULKAN_INSTANCE_FUNCTIONS(DO)                                      \
  DO(vkCreateDevice)                                                           \
  DO(vkCreateSemaphore)                                                        \
  DO(vkDestroyDevice)                                                          \
  DO(vkDestroyInstance)                                                        \
  DO(vkDestroySemaphore)                                                       \
  DO(vkEnumerateDeviceExtensionProperties)                                     \
  DO(vkEnumeratePhysicalDevices)                                               \
  DO(vkGetPhysicalDeviceFeatures2)                                             \
  DO(vkGetPhysicalDeviceProperties)                                            \
  DO(vkGetPhysicalDeviceQueueFamilyProperties)

#define DECLARE_VULKAN_FUNCTION(NAME) inline PFN_##NAME NAME = nullptr;
FOR_VULKAN_GLOBAL_FUNCTIONS(DECLARE_VULKAN_FUNCTION)
FOR_VULKAN_INSTANCE_FUNCTIONS(DECLARE_VULKAN_FUNCTION)
#undef DECLARE_VULKAN_FUNCTION

namespace vlk {

#ifdef __linux__
using PlatformHandle = int;
#define PLATFORM_EXTERNAL_SEMAPHORE_HANDLE_TYPE                                \
  VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT
#else
using PlatformHandle = void *;
#define PLATFORM_EXTERNAL_SEMAPHORE_HANDLE_TYPE                                \
  VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
#endif

constexpr uint32_t REQUIRED_API_VERSION = VK_API_VERSION_1_2;

bool load_platform_functions(VkInstance instance);

const char *get_platform_external_memory_extension_name();
const char *get_platform_external_semaphore_extension_name();

bool initialize_loader();

VkInstance create_instance();

void destroy_instance(VkInstance instance);

std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance);

std::vector<VkExtensionProperties>
get_device_extension_properties(VkPhysicalDevice physical_device);

bool has_timeline_semaphore_support(VkPhysicalDevice physical_device);

uint32_t get_queue_family_index(VkPhysicalDevice physical_device);

VkDevice create_device(VkPhysicalDevice physical_device,
                       uint32_t queue_family_index);

void destroy_device(VkDevice device);

VkSemaphore create_export_semaphore(VkDevice device, VkSemaphoreType type);

void destroy_semaphore(VkDevice device, VkSemaphore semaphore);

PlatformHandle get_semaphore_platform_handle(VkDevice device,
                                             VkSemaphore semaphore);

} // namespace vlk
