/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <wrl.h>

#include <level_zero/ze_api.h>

#include <string>

template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace dx {

std::string hr_to_string(HRESULT result);

ze_external_semaphore_ext_handle_t
import_fence(ze_device_handle_t l0_device, HANDLE shared_handle,
             ze_external_semaphore_ext_flags_t type);

} // namespace dx
