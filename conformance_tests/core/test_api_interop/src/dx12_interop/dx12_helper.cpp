/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx12_interop/dx12_helper.hpp"

#include "logging/logging.hpp"

namespace dx12 {

D3D12_RESOURCE_DESC get_buffer_desc(size_t memory_size) {
  return {.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
          .Alignment = 0,
          .Width = memory_size,
          .Height = 1,
          .DepthOrArraySize = 1,
          .MipLevels = 1,
          .Format = DXGI_FORMAT_UNKNOWN,
          .SampleDesc = {.Count = 1, .Quality = 0},
          .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
}

D3D12_RESOURCE_DESC get_texture_2d_desc(DXGI_FORMAT format, UINT64 width,
                                        UINT height) {
  return {.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
          .Alignment = 0,
          .Width = width,
          .Height = height,
          .DepthOrArraySize = 1,
          .MipLevels = 1,
          .Format = format,
          .SampleDesc = {.Count = 1, .Quality = 0},
          .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN};
}

ComPtr<ID3D12Fence> create_fence(const ComPtr<ID3D12Device> &device,
                                 bool exportable) {
  ComPtr<ID3D12Fence> fence;
  if (HRESULT hr = device->CreateFence(
          0, exportable ? D3D12_FENCE_FLAG_SHARED : D3D12_FENCE_FLAG_NONE,
          IID_PPV_ARGS(&fence));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateFence failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 fence.");
  }

  return fence;
}

ComPtr<ID3D12Heap> create_heap(const ComPtr<ID3D12Device> &device, size_t size,
                               size_t alignment, bool exportable) {
  const D3D12_HEAP_DESC heap_desc = {
      .SizeInBytes = size,
      .Properties = {.Type = D3D12_HEAP_TYPE_DEFAULT},
      .Alignment = alignment,
      .Flags = exportable ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE};

  ComPtr<ID3D12Heap> heap;
  if (HRESULT hr = device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateHeap failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 heap.");
  }

  return heap;
}

D3D12_RESOURCE_ALLOCATION_INFO
get_placed_resource_alloc_info(const ComPtr<ID3D12Device> &device,
                               size_t memory_size) {
  const D3D12_RESOURCE_DESC desc = get_buffer_desc(memory_size);
  return device->GetResourceAllocationInfo(0, 1, &desc);
}

ComPtr<ID3D12Resource>
create_placed_resource(const ComPtr<ID3D12Device> &device,
                       const ComPtr<ID3D12Heap> &heap, size_t size,
                       size_t offset) {
  const D3D12_RESOURCE_DESC desc = get_buffer_desc(size);

  ComPtr<ID3D12Resource> placed_resource;
  if (HRESULT hr = device->CreatePlacedResource(
          heap.Get(), offset, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          IID_PPV_ARGS(&placed_resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreatePlacedResource failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 placed resource.");
  }

  return placed_resource;
}

D3D12_RESOURCE_ALLOCATION_INFO
get_placed_texture_2d_alloc_info(const ComPtr<ID3D12Device> &device,
                                 DXGI_FORMAT format, UINT64 width,
                                 UINT height) {
  const D3D12_RESOURCE_DESC desc = get_texture_2d_desc(format, width, height);
  return device->GetResourceAllocationInfo(0, 1, &desc);
}

ComPtr<ID3D12Resource>
create_placed_texture_2d(const ComPtr<ID3D12Device> &device,
                         const ComPtr<ID3D12Heap> &heap, DXGI_FORMAT format,
                         UINT64 width, UINT height, size_t offset) {
  const D3D12_RESOURCE_DESC desc = get_texture_2d_desc(format, width, height);

  ComPtr<ID3D12Resource> placed_resource;
  if (HRESULT hr = device->CreatePlacedResource(
          heap.Get(), offset, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          IID_PPV_ARGS(&placed_resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreatePlacedResource (texture2D) failed "
                 "with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 placed texture2D.");
  }

  return placed_resource;
}

ComPtr<ID3D12Resource> create_committed_resource(
    const ComPtr<ID3D12Device> &device, D3D12_HEAP_TYPE heap_type,
    D3D12_RESOURCE_STATES state, size_t size, bool exportable) {
  const D3D12_HEAP_PROPERTIES heap_props = {
      .Type = heap_type, .CreationNodeMask = 1, .VisibleNodeMask = 1};

  const D3D12_RESOURCE_DESC desc = get_buffer_desc(size);

  ComPtr<ID3D12Resource> resource;
  if (HRESULT hr = device->CreateCommittedResource(
          &heap_props,
          exportable ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE, &desc,
          state, nullptr, IID_PPV_ARGS(&resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommittedResource failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 committed resource.");
  }

  return resource;
}

ComPtr<ID3D12Resource>
create_committed_texture_2d(const ComPtr<ID3D12Device> &device,
                            DXGI_FORMAT format, UINT64 width, UINT height,
                            bool exportable) {
  const D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT,
                                            .CreationNodeMask = 1,
                                            .VisibleNodeMask = 1};

  const D3D12_RESOURCE_DESC desc = get_texture_2d_desc(format, width, height);

  ComPtr<ID3D12Resource> resource;
  if (HRESULT hr = device->CreateCommittedResource(
          &heap_props,
          exportable ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE, &desc,
          D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommittedResource (texture2D) failed "
                 "with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 committed texture2D.");
  }

  return resource;
}

HANDLE create_shared_handle(const ComPtr<ID3D12Device> &device,
                            ID3D12DeviceChild *object) {
  HANDLE handle = {};
  if (HRESULT hr = device->CreateSharedHandle(object, nullptr, GENERIC_ALL,
                                              nullptr, &handle);
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateSharedHandle failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error(
        "Failed to create shared handle for a DX12 object.");
  }

  return handle;
}

CommandBundle create_command_bundle(const ComPtr<ID3D12Device> &device) {
  CommandBundle bundle;

  const D3D12_COMMAND_QUEUE_DESC cmdq_desc = {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT};
  if (HRESULT hr = device->CreateCommandQueue(&cmdq_desc,
                                              IID_PPV_ARGS(&bundle.cmd_queue));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommandQueue failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command queue.");
  }

  if (HRESULT hr = device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&bundle.cmd_allocator));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommandAllocator failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command allocator.");
  }

  if (HRESULT hr = device->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, bundle.cmd_allocator.Get(),
          nullptr, IID_PPV_ARGS(&bundle.cmd_list));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommandList failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command list.");
  }

  return bundle;
}

void wait_for_fence(const ComPtr<ID3D12Fence> &fence, uint64_t wait_value) {
  if (fence->GetCompletedValue() < wait_value) {
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (event == NULL) {
      DWORD hr = GetLastError();
      LOG_ERROR << "CreateEvent failed with: " << dx::hr_to_string(hr);
      throw std::runtime_error("Failed to create Windows event.");
    }
    if (HRESULT hr = fence->SetEventOnCompletion(wait_value, event);
        FAILED(hr)) {
      LOG_ERROR << "ID3D12Fence::SetEventOnCompletion failed with: "
                << dx::hr_to_string(hr);
      throw std::runtime_error("Failed to set event on fence completion.");
    }
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }
}

void execute_and_sync_command_bundle(const ComPtr<ID3D12Device> &device,
                                     const CommandBundle &cmd_bundle) {
  ID3D12CommandList *lists[] = {cmd_bundle.cmd_list.Get()};
  cmd_bundle.cmd_queue->ExecuteCommandLists(1, lists);

  ComPtr<ID3D12Fence> fence = dx12::create_fence(device);
  cmd_bundle.cmd_queue->Signal(fence.Get(), 1);

  wait_for_fence(fence, 1);
}

UINT get_format_plane_count(const ComPtr<ID3D12Device> &device,
                            DXGI_FORMAT format) {
  D3D12_FEATURE_DATA_FORMAT_INFO info{.Format = format, .PlaneCount = 0};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &info,
                                         sizeof(info)))) {
    return 1; // unsupported/unknown -> treat as single plane
  }
  return info.PlaneCount;
}

} // namespace dx12
