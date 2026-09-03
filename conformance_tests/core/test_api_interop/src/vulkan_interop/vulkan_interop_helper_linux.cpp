/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "vulkan_interop/vulkan_interop_helper.hpp"

ze_external_semaphore_ext_handle_t
import_semaphore(ze_device_handle_t l0_device, VkDevice vk_device,
                 VkSemaphore vk_semaphore, VkSemaphoreType type) {
  const ze_external_semaphore_fd_ext_desc_t external_semaphore_fd_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_FD_EXT_DESC,
      .fd = vlk::get_semaphore_platform_handle(vk_device, vk_semaphore)};

  const ze_external_semaphore_ext_desc_t external_semaphore_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC,
      .pNext = &external_semaphore_fd_desc,
      .flags = static_cast<ze_external_semaphore_ext_flags_t>(
          (type == VK_SEMAPHORE_TYPE_TIMELINE)
              ? ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_FD
              : ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_FD)};

  return lzt::import_external_semaphore(l0_device, &external_semaphore_desc);
}
