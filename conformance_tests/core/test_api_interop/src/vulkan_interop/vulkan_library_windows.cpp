/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "vulkan_interop/vulkan_library.hpp"

#include "logging/logging.hpp"

namespace vlk {

const char *get_platform_external_memory_extension_name() {
  return VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
}
const char *get_platform_external_semaphore_extension_name() {
  return VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
}

bool load_platform_functions(VkInstance instance) {
  vkGetSemaphoreWin32HandleKHR =
      reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(
          vkGetInstanceProcAddr(instance, "vkGetSemaphoreWin32HandleKHR"));
  if (vkGetSemaphoreWin32HandleKHR == nullptr) {
    LOG_WARNING
        << "Failed to load Vulkan function: vkGetSemaphoreWin32HandleKHR";
    return false;
  }

  return true;
}

PlatformHandle get_semaphore_platform_handle(VkDevice device,
                                             VkSemaphore semaphore) {
  const VkSemaphoreGetWin32HandleInfoKHR get_win32_handle_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR,
      .semaphore = semaphore,
      .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR};

  void *handle = nullptr;
  VK_CHECK(
      vkGetSemaphoreWin32HandleKHR(device, &get_win32_handle_info, &handle));

  return handle;
}

} // namespace vlk
