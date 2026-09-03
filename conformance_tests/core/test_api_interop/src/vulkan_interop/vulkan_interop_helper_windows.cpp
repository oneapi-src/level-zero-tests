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
  const ze_external_semaphore_win32_ext_desc_t external_semaphore_win32_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC,
      .handle = vlk::get_semaphore_platform_handle(vk_device, vk_semaphore)};

  const ze_external_semaphore_ext_desc_t external_semaphore_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC,
      .pNext = &external_semaphore_win32_desc,
      .flags = static_cast<ze_external_semaphore_ext_flags_t>(
          (type == VK_SEMAPHORE_TYPE_TIMELINE)
              ? ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_VK_TIMELINE_SEMAPHORE_WIN32
              : ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_OPAQUE_WIN32)};

  return lzt::import_external_semaphore(l0_device, &external_semaphore_desc);
}
