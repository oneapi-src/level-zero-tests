/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx_common.hpp"

#include "test_harness/test_harness_device.hpp"
#include "test_harness/test_harness_image.hpp"

namespace lzt = level_zero_tests;

namespace dx {

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

ze_external_semaphore_ext_handle_t
import_fence(ze_device_handle_t l0_device, HANDLE shared_handle,
             ze_external_semaphore_ext_flags_t type) {
  ze_external_semaphore_win32_ext_desc_t external_semaphore_win32_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC,
      .handle = shared_handle,
  };

  ze_external_semaphore_ext_desc_t external_semaphore_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC,
      .pNext = &external_semaphore_win32_desc,
      .flags = type};

  return lzt::import_external_semaphore(l0_device, &external_semaphore_desc);
}

ze_image_handle_t import_image(HANDLE shared_handle,
                               ze_external_memory_type_flags_t type,
                               uint64_t width, uint32_t height,
                               DXGI_FORMAT format) {
  const ze_external_memory_import_win32_handle_t import_handle = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32,
      .flags = type,
      .handle = shared_handle};

  const ze_image_desc_t image_desc = {.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC,
                                      .pNext = &import_handle,
                                      .type = ZE_IMAGE_TYPE_2D,
                                      .format = dx::to_ze_image_format(format),
                                      .width = width,
                                      .height = height,
                                      .depth = 1};

  return lzt::create_ze_image(image_desc);
}

ze_image_handle_t create_plane_view(ze_device_handle_t l0_device,
                                    ze_image_handle_t image, uint32_t index,
                                    ze_image_format_layout_t layout,
                                    uint64_t width, uint32_t height) {
  const ze_image_view_planar_ext_desc_t plane_ext_desc = {
      .stype = ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC,
      .planeIndex = index};
  const ze_image_desc_t desc = {.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC,
                                .pNext = &plane_ext_desc,
                                .type = ZE_IMAGE_TYPE_2D,
                                .format = {.layout = layout},
                                .width = width,
                                .height = height,
                                .depth = 1};

  return lzt::create_ze_image_view_ext(l0_device, &desc, image);
}

ze_image_format_t to_ze_image_format(DXGI_FORMAT format) {
  switch (format) {
  case DXGI_FORMAT_R8_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8,  ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8,  ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8,  ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8,  ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8G8_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,  ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,  ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8G8_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,  ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,  ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8G8_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,  ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,  ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8G8_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,  ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,  ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R8G8B8A8_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R8G8B8A8_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R8G8B8A8_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R8G8B8A8_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R16_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16, ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16, ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R16G16B16A16_UNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_UNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R16G16B16A16_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R16G16B16A16_SNORM:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_SNORM,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R16G16B16A16_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R32_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_D32_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_D, ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X, ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32G32_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32G32_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32G32_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,    ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_X,    ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_FLOAT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R32G32B32A32_UINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return {ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_TYPE_SINT,
            ZE_IMAGE_FORMAT_SWIZZLE_R,          ZE_IMAGE_FORMAT_SWIZZLE_G,
            ZE_IMAGE_FORMAT_SWIZZLE_B,          ZE_IMAGE_FORMAT_SWIZZLE_A};

  // Media formats - type and swizzle are ignored
  case DXGI_FORMAT_NV12:
    return {ZE_IMAGE_FORMAT_LAYOUT_NV12, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_X,   ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X,   ZE_IMAGE_FORMAT_SWIZZLE_X};
  case DXGI_FORMAT_P010:
    return {ZE_IMAGE_FORMAT_LAYOUT_P010, ZE_IMAGE_FORMAT_TYPE_UINT,
            ZE_IMAGE_FORMAT_SWIZZLE_X,   ZE_IMAGE_FORMAT_SWIZZLE_X,
            ZE_IMAGE_FORMAT_SWIZZLE_X,   ZE_IMAGE_FORMAT_SWIZZLE_X};
  }

  throw std::runtime_error("Unimplemented format.");
}

} // namespace dx
