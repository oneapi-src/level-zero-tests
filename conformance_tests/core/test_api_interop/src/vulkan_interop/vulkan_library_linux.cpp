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
  return VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
}
const char *get_platform_external_semaphore_extension_name() {
  return VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
}

bool load_platform_functions(VkInstance instance) {
  vkGetSemaphoreFdKHR = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(
      vkGetInstanceProcAddr(instance, "vkGetSemaphoreFdKHR"));
  if (vkGetSemaphoreFdKHR == nullptr) {
    LOG_WARNING << "Failed to load Vulkan function: vkGetSemaphoreFdKHR";
    return false;
  }

  return true;
}

PlatformHandle get_semaphore_platform_handle(VkDevice device,
                                             VkSemaphore semaphore) {
  const VkSemaphoreGetFdInfoKHR get_fd_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
      .semaphore = semaphore,
      .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR};

  int fd = -1;
  VK_CHECK(vkGetSemaphoreFdKHR(device, &get_fd_info, &fd));

  return fd;
}

} // namespace vlk
