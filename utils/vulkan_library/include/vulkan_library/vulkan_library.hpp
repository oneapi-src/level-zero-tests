/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
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
  DO(vkAllocateCommandBuffers)                                                 \
  DO(vkAllocateDescriptorSets)                                                 \
  DO(vkAllocateMemory)                                                         \
  DO(vkBeginCommandBuffer)                                                     \
  DO(vkBindBufferMemory)                                                       \
  DO(vkBindImageMemory)                                                        \
  DO(vkCmdBindDescriptorSets)                                                  \
  DO(vkCmdBindPipeline)                                                        \
  DO(vkCmdCopyBuffer)                                                          \
  DO(vkCmdCopyBufferToImage)                                                   \
  DO(vkCmdCopyImageToBuffer)                                                   \
  DO(vkCmdDispatch)                                                            \
  DO(vkCmdFillBuffer)                                                          \
  DO(vkCmdPipelineBarrier)                                                     \
  DO(vkCmdPushConstants)                                                       \
  DO(vkCreateBuffer)                                                           \
  DO(vkCreateCommandPool)                                                      \
  DO(vkCreateComputePipelines)                                                 \
  DO(vkCreateDescriptorPool)                                                   \
  DO(vkCreateDescriptorSetLayout)                                              \
  DO(vkCreateDevice)                                                           \
  DO(vkCreateFence)                                                            \
  DO(vkCreateImage)                                                            \
  DO(vkCreatePipelineLayout)                                                   \
  DO(vkCreateSemaphore)                                                        \
  DO(vkCreateShaderModule)                                                     \
  DO(vkDestroyBuffer)                                                          \
  DO(vkDestroyCommandPool)                                                     \
  DO(vkDestroyDescriptorPool)                                                  \
  DO(vkDestroyDescriptorSetLayout)                                             \
  DO(vkDestroyDevice)                                                          \
  DO(vkDestroyFence)                                                           \
  DO(vkDestroyImage)                                                           \
  DO(vkDestroyInstance)                                                        \
  DO(vkDestroyPipeline)                                                        \
  DO(vkDestroyPipelineLayout)                                                  \
  DO(vkDestroySemaphore)                                                       \
  DO(vkDestroyShaderModule)                                                    \
  DO(vkEndCommandBuffer)                                                       \
  DO(vkEnumerateDeviceExtensionProperties)                                     \
  DO(vkEnumeratePhysicalDevices)                                               \
  DO(vkFreeMemory)                                                             \
  DO(vkGetBufferMemoryRequirements2)                                           \
  DO(vkGetDeviceProcAddr)                                                      \
  DO(vkGetDeviceQueue)                                                         \
  DO(vkGetImageMemoryRequirements2)                                            \
  DO(vkGetImageSubresourceLayout)                                              \
  DO(vkGetPhysicalDeviceFeatures2)                                             \
  DO(vkGetPhysicalDeviceImageFormatProperties2)                                \
  DO(vkGetPhysicalDeviceMemoryProperties2)                                     \
  DO(vkGetPhysicalDeviceProperties)                                            \
  DO(vkGetPhysicalDeviceQueueFamilyProperties)                                 \
  DO(vkGetPhysicalDeviceQueueFamilyProperties2)                                \
  DO(vkMapMemory)                                                              \
  DO(vkQueueSubmit)                                                            \
  DO(vkQueueWaitIdle)                                                          \
  DO(vkUnmapMemory)                                                            \
  DO(vkUpdateDescriptorSets)                                                   \
  DO(vkWaitForFences)

#define DECLARE_VULKAN_FUNCTION(NAME) inline PFN_##NAME NAME = nullptr;
FOR_VULKAN_GLOBAL_FUNCTIONS(DECLARE_VULKAN_FUNCTION)
FOR_VULKAN_INSTANCE_FUNCTIONS(DECLARE_VULKAN_FUNCTION)
#undef DECLARE_VULKAN_FUNCTION

namespace level_zero_tests::vlk {

#ifdef __linux__
using PlatformHandle = int;
constexpr auto PLATFORM_EXTERNAL_SEMAPHORE_HANDLE_TYPE =
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#else
using PlatformHandle = void *;
constexpr auto PLATFORM_EXTERNAL_SEMAPHORE_HANDLE_TYPE =
    VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#endif

constexpr uint32_t REQUIRED_API_VERSION = VK_API_VERSION_1_2;

bool load_platform_functions(VkInstance instance);

const char *get_platform_external_memory_extension_name();
const char *get_platform_external_semaphore_extension_name();

bool initialize_loader();

// For an instance not created by create_instance. Requires a successful
// initialize_loader() and an instance with apiVersion >= 1.1. Platform
// functions are not loaded; call load_platform_functions for those.
bool load_instance_functions(VkInstance instance);

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

} // namespace level_zero_tests::vlk
