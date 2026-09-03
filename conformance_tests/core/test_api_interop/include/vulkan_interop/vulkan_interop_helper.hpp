/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "vulkan_interop/vulkan_library.hpp"

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>

ze_external_semaphore_ext_handle_t
import_semaphore(ze_device_handle_t l0_device, VkDevice vk_device,
                 VkSemaphore vk_semaphore, VkSemaphoreType type);
