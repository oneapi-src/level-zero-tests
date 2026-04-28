/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "../dx_common.hpp"

#include <d3d11.h>
#include <d3d11_4.h>

namespace dx11 {

ComPtr<ID3D11Fence> create_fence(const ComPtr<ID3D11Device5> &device5,
                                 bool exportable = false);

HANDLE create_shared_handle(const ComPtr<ID3D11Fence> &fence);

void wait_for_fence(const ComPtr<ID3D11Fence> &fence, uint64_t wait_value);

} // namespace dx11
