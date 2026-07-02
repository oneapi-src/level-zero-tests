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

#include <gtest/gtest.h>
#include <level_zero/ze_api.h>

#include <string>

template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace dx {

std::string hr_to_string(HRESULT result);

ze_external_semaphore_ext_handle_t
import_fence(ze_device_handle_t l0_device, HANDLE shared_handle,
             ze_external_semaphore_ext_flags_t type);

ze_external_semaphore_ext_handle_t
import_fence_by_name(ze_device_handle_t l0_device, LPCWSTR name,
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

struct DXImageTestParamPrinter {
  template <class ParamType>
  std::string
  operator()(const ::testing::TestParamInfo<ParamType> &info) const {
    switch (info.param) {
    case DXGI_FORMAT_R8_UNORM:
      return "R8_UNORM";
    case DXGI_FORMAT_R8_UINT:
      return "R8_UINT";
    case DXGI_FORMAT_R8_SNORM:
      return "R8_SNORM";
    case DXGI_FORMAT_R8_SINT:
      return "R8_SINT";
    case DXGI_FORMAT_R8G8_UNORM:
      return "R8G8_UNORM";
    case DXGI_FORMAT_R8G8_UINT:
      return "R8G8_UINT";
    case DXGI_FORMAT_R8G8_SNORM:
      return "R8G8_SNORM";
    case DXGI_FORMAT_R8G8_SINT:
      return "R8G8_SINT";
    case DXGI_FORMAT_R8G8B8A8_UNORM:
      return "R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UINT:
      return "R8G8B8A8_UINT";
    case DXGI_FORMAT_R8G8B8A8_SNORM:
      return "R8G8B8A8_SNORM";
    case DXGI_FORMAT_R8G8B8A8_SINT:
      return "R8G8B8A8_SINT";
    case DXGI_FORMAT_R16_FLOAT:
      return "R16_FLOAT";
    case DXGI_FORMAT_R16_UNORM:
      return "R16_UNORM";
    case DXGI_FORMAT_R16_UINT:
      return "R16_UINT";
    case DXGI_FORMAT_R16_SNORM:
      return "R16_SNORM";
    case DXGI_FORMAT_R16_SINT:
      return "R16_SINT";
    case DXGI_FORMAT_R16G16_FLOAT:
      return "R16G16_FLOAT";
    case DXGI_FORMAT_R16G16_UNORM:
      return "R16G16_UNORM";
    case DXGI_FORMAT_R16G16_UINT:
      return "R16G16_UINT";
    case DXGI_FORMAT_R16G16_SNORM:
      return "R16G16_SNORM";
    case DXGI_FORMAT_R16G16_SINT:
      return "R16G16_SINT";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return "R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R16G16B16A16_UNORM:
      return "R16G16B16A16_UNORM";
    case DXGI_FORMAT_R16G16B16A16_UINT:
      return "R16G16B16A16_UINT";
    case DXGI_FORMAT_R16G16B16A16_SNORM:
      return "R16G16B16A16_SNORM";
    case DXGI_FORMAT_R16G16B16A16_SINT:
      return "R16G16B16A16_SINT";
    case DXGI_FORMAT_R32_FLOAT:
      return "R32_FLOAT";
    case DXGI_FORMAT_R32_UINT:
      return "R32_UINT";
    case DXGI_FORMAT_R32_SINT:
      return "R32_SINT";
    case DXGI_FORMAT_D32_FLOAT:
      return "D32_FLOAT";
    case DXGI_FORMAT_R32G32_FLOAT:
      return "R32G32_FLOAT";
    case DXGI_FORMAT_R32G32_UINT:
      return "R32G32_UINT";
    case DXGI_FORMAT_R32G32_SINT:
      return "R32G32_SINT";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return "R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return "R32G32B32A32_UINT";
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return "R32G32B32A32_SINT";
    }

    return "Unknown";
  }
};
