/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "dx_common.hpp"

#include <d3d12.h>

namespace dx12 {

ComPtr<ID3D12Fence> create_fence(const ComPtr<ID3D12Device> &device,
                                 bool exportable = false);

ComPtr<ID3D12Heap> create_heap(const ComPtr<ID3D12Device> &device, size_t size,
                               size_t alignment, bool exportable = false);

D3D12_RESOURCE_ALLOCATION_INFO
get_placed_resource_alloc_info(const ComPtr<ID3D12Device> &device,
                               size_t memory_size);

ComPtr<ID3D12Resource>
create_placed_resource(const ComPtr<ID3D12Device> &device,
                       const ComPtr<ID3D12Heap> &heap, size_t size,
                       size_t offset = 0);

D3D12_RESOURCE_ALLOCATION_INFO
get_placed_texture_2d_alloc_info(const ComPtr<ID3D12Device> &device,
                                 DXGI_FORMAT format, UINT64 width, UINT height);

ComPtr<ID3D12Resource>
create_placed_texture_2d(const ComPtr<ID3D12Device> &device,
                         const ComPtr<ID3D12Heap> &heap, DXGI_FORMAT format,
                         UINT64 width, UINT height, size_t offset = 0);

ComPtr<ID3D12Resource> create_committed_resource(
    const ComPtr<ID3D12Device> &device, D3D12_HEAP_TYPE heap_type,
    D3D12_RESOURCE_STATES state, size_t size, bool exportable = false);

ComPtr<ID3D12Resource>
create_committed_texture_2d(const ComPtr<ID3D12Device> &device,
                            DXGI_FORMAT format, UINT64 width, UINT height,
                            bool exportable = false);

HANDLE create_shared_handle(const ComPtr<ID3D12Device> &device,
                            ID3D12DeviceChild *object);

struct CommandBundle {
  ComPtr<ID3D12CommandQueue> cmd_queue;
  ComPtr<ID3D12CommandAllocator> cmd_allocator;
  ComPtr<ID3D12GraphicsCommandList> cmd_list;
};

CommandBundle create_command_bundle(const ComPtr<ID3D12Device> &device);

void wait_for_fence(const ComPtr<ID3D12Fence> &fence, uint64_t wait_value);

void execute_and_sync_command_bundle(const ComPtr<ID3D12Device> &device,
                                     const CommandBundle &cmd_bundle);

UINT get_format_plane_count(const ComPtr<ID3D12Device> &device,
                            DXGI_FORMAT format);

} // namespace dx12
