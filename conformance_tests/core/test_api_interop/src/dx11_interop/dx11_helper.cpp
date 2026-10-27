/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx11_interop/dx11_helper.hpp"

#include "logging/logging.hpp"

namespace dx11 {

namespace {

UINT cpu_access_flags(texture_usage usage) {
  switch (usage) {
  case texture_usage::upload:
    return D3D11_CPU_ACCESS_WRITE;
  case texture_usage::readback:
    return D3D11_CPU_ACCESS_READ;
  default:
    return 0u;
  }
}

} // namespace

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

HANDLE create_shared_handle(const ComPtr<ID3D11Fence> &fence, LPCWSTR name) {
  HANDLE handle = {};
  if (HRESULT hr =
          fence->CreateSharedHandle(nullptr, GENERIC_ALL, name, &handle);
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

ComPtr<ID3D11Texture2D> create_texture_2d(const ComPtr<ID3D11Device> &device,
                                          texture_usage usage,
                                          DXGI_FORMAT format, UINT width,
                                          UINT height, UINT extra_bind_flags) {
  const bool shared = usage == texture_usage::shared;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = shared ? D3D11_USAGE_DEFAULT : D3D11_USAGE_STAGING;
  desc.BindFlags =
      (shared ? D3D11_BIND_SHADER_RESOURCE : 0u) | extra_bind_flags;
  desc.CPUAccessFlags = cpu_access_flags(usage);
  desc.MiscFlags = shared ? (D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
                             D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)
                          : 0u;

  ComPtr<ID3D11Texture2D> texture;
  if (HRESULT hr = device->CreateTexture2D(&desc, nullptr, &texture);
      FAILED(hr)) {
    LOG_ERROR << "ID3D11Device::CreateTexture2D failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error("Failed to create DX11 texture.");
  }

  return texture;
}

HANDLE create_shared_handle(const ComPtr<ID3D11Texture2D> &texture) {
  ComPtr<IDXGIResource1> dxgi_resource;
  if (HRESULT hr = texture.As(&dxgi_resource); FAILED(hr)) {
    LOG_ERROR << "Failed to query IDXGIResource1: " << dx::hr_to_string(hr);
    throw std::runtime_error(
        "Failed to query IDXGIResource1 from DX11 texture.");
  }

  HANDLE handle = nullptr;
  if (HRESULT hr = dxgi_resource->CreateSharedHandle(nullptr, GENERIC_ALL,
                                                     nullptr, &handle);
      FAILED(hr)) {
    LOG_ERROR << "IDXGIResource1::CreateSharedHandle failed with: "
              << dx::hr_to_string(hr);
    throw std::runtime_error(
        "Failed to create shared handle for a DX11 texture.");
  }

  return handle;
}

} // namespace dx11
