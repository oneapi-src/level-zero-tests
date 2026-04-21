/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx12_helper.hpp"

#include "logging/logging.hpp"

namespace dx12 {

std::string hr_to_string(HRESULT result) {
  char *msg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR)&msg, 0, nullptr);

  std::string str = msg ? msg : "Unknown error";
  if (msg)
    LocalFree(msg);
  return str;
}

ComPtr<ID3D12Fence> create_fence(const ComPtr<ID3D12Device> &device,
                                 bool exportable) {
  ComPtr<ID3D12Fence> fence;
  if (HRESULT hr = device->CreateFence(
          0, exportable ? D3D12_FENCE_FLAG_SHARED : D3D12_FENCE_FLAG_NONE,
          IID_PPV_ARGS(&fence));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateFence failed with: " << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 fence.");
  }

  return fence;
}

ComPtr<ID3D12Heap> create_heap(const ComPtr<ID3D12Device> &device, size_t size,
                               bool exportable) {
  const D3D12_HEAP_DESC heap_desc = {
      .SizeInBytes = size,
      .Properties = {.Type = D3D12_HEAP_TYPE_DEFAULT},
      .Flags = exportable ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE};

  ComPtr<ID3D12Heap> heap;
  if (HRESULT hr = device->CreateHeap(&heap_desc, IID_PPV_ARGS(&heap));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateHeap failed with: " << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 heap.");
  }

  return heap;
}

ComPtr<ID3D12Resource>
create_placed_resource(const ComPtr<ID3D12Device> &device,
                       const ComPtr<ID3D12Heap> &heap, size_t size,
                       size_t offset) {
  const D3D12_RESOURCE_DESC desc = {.Dimension =
                                        D3D12_RESOURCE_DIMENSION_BUFFER,
                                    .Width = size,
                                    .Height = 1,
                                    .DepthOrArraySize = 1,
                                    .MipLevels = 1,
                                    .Format = DXGI_FORMAT_UNKNOWN,
                                    .SampleDesc = {.Count = 1, .Quality = 0},
                                    .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};

  ComPtr<ID3D12Resource> placed_resource;
  if (HRESULT hr = device->CreatePlacedResource(
          heap.Get(), offset, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          IID_PPV_ARGS(&placed_resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreatePlacedResource failed with: "
              << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 placed resource.");
  }

  return placed_resource;
}

ComPtr<ID3D12Resource> create_committed_resource(
    const ComPtr<ID3D12Device> &device, D3D12_HEAP_TYPE heap_type,
    D3D12_RESOURCE_STATES state, size_t size, bool exportable) {
  const D3D12_HEAP_PROPERTIES heap_props = {
      .Type = heap_type, .CreationNodeMask = 1, .VisibleNodeMask = 1};

  const D3D12_RESOURCE_DESC resource_desc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Width = size,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc = {.Count = 1, .Quality = 0},
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};

  ComPtr<ID3D12Resource> resource;
  if (HRESULT hr = device->CreateCommittedResource(
          &heap_props,
          exportable ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE,
          &resource_desc, state, nullptr, IID_PPV_ARGS(&resource));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommittedResource failed with: "
              << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 committed resource.");
  }

  return resource;
}

HANDLE create_shared_handle(const ComPtr<ID3D12Device> &device,
                            ID3D12DeviceChild *object) {
  HANDLE handle = {};
  if (FAILED(device->CreateSharedHandle(object, nullptr, GENERIC_ALL, nullptr,
                                        &handle))) {
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
              << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command queue.");
  }

  if (HRESULT hr = device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&bundle.cmd_allocator));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommandAllocator failed with: "
              << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command allocator.");
  }

  if (HRESULT hr = device->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, bundle.cmd_allocator.Get(),
          nullptr, IID_PPV_ARGS(&bundle.cmd_list));
      FAILED(hr)) {
    LOG_ERROR << "ID3D12Device::CreateCommandList failed with: "
              << hr_to_string(hr);
    throw std::runtime_error("Failed to create DX12 command list.");
  }

  return bundle;
}

void execute_and_sync_command_bundle(const ComPtr<ID3D12Device> &device,
                                     const CommandBundle &cmd_bundle) {
  ID3D12CommandList *lists[] = {cmd_bundle.cmd_list.Get()};
  cmd_bundle.cmd_queue->ExecuteCommandLists(1, lists);

  ComPtr<ID3D12Fence> fence = dx12::create_fence(device);
  cmd_bundle.cmd_queue->Signal(fence.Get(), 1);

  if (fence->GetCompletedValue() < 1) {
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (event == NULL) {
      DWORD hr = GetLastError();
      LOG_ERROR << "CreateEvent failed with: " << hr_to_string(hr);
      throw std::runtime_error("Failed to create Windows event.");
    }
    fence->SetEventOnCompletion(1, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }
}

} // namespace dx12
