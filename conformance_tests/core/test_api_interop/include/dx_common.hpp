/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <dxgi.h>
#include <wrl.h>

#include <level_zero/ze_api.h>

#include <string>

template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace dx {

std::string hr_to_string(HRESULT result);

ze_external_semaphore_ext_handle_t
import_fence(ze_device_handle_t l0_device, HANDLE shared_handle,
             ze_external_semaphore_ext_flags_t type);

ze_image_handle_t import_image(HANDLE shared_handle,
                               ze_external_memory_type_flags_t type,
                               uint64_t width, uint32_t height,
                               DXGI_FORMAT format);

ze_image_handle_t create_plane_view(ze_device_handle_t l0_device,
                                    ze_image_handle_t image, uint32_t index,
                                    ze_image_format_layout_t layout,
                                    uint64_t width, uint32_t height);

ze_image_format_t to_ze_image_format(DXGI_FORMAT format);

} // namespace dx
