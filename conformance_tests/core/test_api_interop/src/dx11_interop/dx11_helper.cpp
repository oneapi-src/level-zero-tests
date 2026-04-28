/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx11_helper.hpp"

#include "logging/logging.hpp"

namespace dx11 {

ComPtr<ID3D11Fence> create_fence(const ComPtr<ID3D11Device5> &device5,
                                 bool exportable) {
  ComPtr<ID3D11Fence> fence;
  if (HRESULT hr = device5->CreateFence(
          0, exportable ? D3D11_FENCE_FLAG_SHARED : D3D11_FENCE_FLAG_NONE,
          IID_PPV_ARGS(&fence));
      FAILED(hr)) {
    LOG_ERROR << "ID3D11Device5::CreateFence failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX11 fence.");
  }

  return fence;
}

HANDLE create_shared_handle(const ComPtr<ID3D11Fence> &fence) {
  HANDLE handle = {};
  if (HRESULT hr =
          fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &handle);
      FAILED(hr)) {
    LOG_ERROR << "ID3D11Fence::CreateSharedHandle failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error(
        "Failed to create shared handle for a DX11 fence.");
  }

  return handle;
}

void wait_for_fence(const ComPtr<ID3D11Fence> &fence, uint64_t wait_value) {
  if (fence->GetCompletedValue() < wait_value) {
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (event == NULL) {
      DWORD hr = GetLastError();
      LOG_ERROR << "CreateEvent failed with: " << dx::hr_to_string(hr);
      throw std::runtime_error("Failed to create Windows event.");
    }
    fence->SetEventOnCompletion(wait_value, event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
  }
}

} // namespace dx11
