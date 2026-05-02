/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"
#include "random/random.hpp"

#include <span>

#ifdef __linux__
struct DX11InteroperabilityTests : ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
#include "dx11_interop/dx11_interop_fixture.hpp"
#endif

namespace {

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenSignalingByL0ThenIsSuccessfullySignaled) {
#ifndef __linux__
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  EXPECT_EQ(fence->GetCompletedValue(), 0);

  uint64_t wait_value = 1;

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  ze_external_semaphore_signal_params_ext_t semaphore_signal_params = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS_EXT,
      .value = wait_value};
  lzt::append_signal_external_semaphore(
      l0_cmd_bundle.list, 1, &external_semaphore_handle,
      &semaphore_signal_params, nullptr, 0, nullptr);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  dx11::wait_for_fence(fence, wait_value);

  EXPECT_EQ(fence->GetCompletedValue(), wait_value);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceSignaledOnDeviceWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  auto l0_event_pool = lzt::create_event_pool(lzt::get_default_context(), 1,
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

  EXPECT_EQ(fence->GetCompletedValue(), 0);

  uint64_t wait_value = 1;

  dx11_device_context4->Signal(fence.Get(), wait_value);
  dx11_device_context4->Flush();

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  ze_external_semaphore_wait_params_ext_t semaphore_wait_params = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WAIT_PARAMS_EXT,
      .value = wait_value};
  lzt::append_wait_external_semaphore(
      l0_cmd_bundle.list, 1, &external_semaphore_handle, &semaphore_wait_params,
      l0_after_wait_event, 0, nullptr);
  lzt::append_wait_on_events(l0_cmd_bundle.list, 1, &l0_after_wait_event);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(fence->GetCompletedValue(), wait_value);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::release_external_semaphore(external_semaphore_handle);
  lzt::destroy_event(l0_after_wait_event);
  lzt::destroy_event_pool(l0_event_pool);
  CloseHandle(fence_shared_handle);
#endif
}

struct DX11InteroperabilityMultiPlanarImageTests
    : public DX11InteroperabilityTests {
  void SetUp() override {
#ifndef __linux__
    DX11InteroperabilityTests::SetUp();
    if (!lzt::image_support(l0_device)) {
      GTEST_SKIP() << "Device doesn't support images.";
    }
#else
    GTEST_SKIP() << "Not supported on Linux";
#endif
  }

#ifndef __linux__
  void upload_planar_texture(const ComPtr<ID3D11Texture2D> &shared_texture,
                             DXGI_FORMAT format, UINT width, UINT height,
                             std::span<const std::byte> plane0_bytes,
                             size_t plane0_row_size, UINT plane0_rows,
                             std::span<const std::byte> plane1_bytes,
                             size_t plane1_row_size, UINT plane1_rows) {
    auto staging = dx11::create_texture_2d(dx11_device, format, width, height);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (HRESULT hr = dx11_device_context->Map(staging.Get(), 0, D3D11_MAP_WRITE,
                                              0, &mapped);
        FAILED(hr)) {
      LOG_ERROR << "ID3D11DeviceContext::Map failed with: "
                << dx::hr_to_string(hr);
      throw std::runtime_error("Failed to Map plane 0 of staging texture.");
    }

    for (UINT row = 0; row < plane0_rows; ++row) {
      std::memcpy(static_cast<uint8_t *>(mapped.pData) + mapped.RowPitch * row,
                  plane0_bytes.data() + plane0_row_size * row, plane0_row_size);
    }

    size_t uv_plane_offset = static_cast<size_t>(mapped.RowPitch) * (height);
    for (UINT row = 0; row < plane1_rows; ++row) {
      std::memcpy(static_cast<uint8_t *>(mapped.pData) + uv_plane_offset +
                      mapped.RowPitch * row,
                  plane1_bytes.data() + plane1_row_size * row, plane1_row_size);
    }

    dx11_device_context->Unmap(staging.Get(), 0);

    ComPtr<IDXGIKeyedMutex> keyed_mutex;
    if (FAILED(shared_texture.As(&keyed_mutex))) {
      throw std::runtime_error(
          "Failed to query IDXGIKeyedMutex on shared texture.");
    }
    if (FAILED(keyed_mutex->AcquireSync(0, INFINITE))) {
      throw std::runtime_error("AcquireSync(0) failed on shared texture.");
    }
    dx11_device_context->CopyResource(shared_texture.Get(), staging.Get());
    dx11_device_context->Flush();
    if (FAILED(keyed_mutex->ReleaseSync(0))) {
      throw std::runtime_error("ReleaseSync(0) failed on shared texture.");
    }
  }
#endif
};

LZT_TEST_F(
    DX11InteroperabilityMultiPlanarImageTests,
    GivenDX11SharedTexture2DWithNV12FormatWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  DXGI_FORMAT format = DXGI_FORMAT_NV12;
  constexpr UINT width = 1024;
  constexpr UINT height = 1024;

  const size_t y_plane_size = static_cast<size_t>(width) * height;
  const size_t uv_plane_size =
      static_cast<size_t>(width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint8_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint8_t>(uv_plane_size, 0);

  auto shared_texture =
      dx11::create_texture_2d(dx11_device, format, width, height, true);

  upload_planar_texture(shared_texture, format, width, height,
                        as_bytes(std::span(src_y_plane_values)), width, height,
                        as_bytes(std::span(src_uv_plane_values)), width,
                        height / 2);

  auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE, width,
      height, format);

  auto y_plane_view = dx::create_plane_view(
      l0_device, imported_image, 0, ZE_IMAGE_FORMAT_LAYOUT_8, width, height);
  auto uv_plane_view =
      dx::create_plane_view(l0_device, imported_image, 1,
                            ZE_IMAGE_FORMAT_LAYOUT_8_8, width / 2, height / 2);

  std::vector<uint8_t> dst_y_plane_values(y_plane_size);
  std::vector<uint8_t> dst_uv_plane_values(uv_plane_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_y_plane_values.data(),
                                y_plane_view, nullptr);
  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_uv_plane_values.data(),
                                uv_plane_view, nullptr);
  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::destroy_ze_image(y_plane_view);
  lzt::destroy_ze_image(uv_plane_view);
  lzt::destroy_ze_image(imported_image);
  CloseHandle(texture_shared_handle);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityMultiPlanarImageTests,
    GivenDX11SharedTexture2DWithP010FormatWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  DXGI_FORMAT format = DXGI_FORMAT_P010;
  constexpr UINT width = 1024;
  constexpr UINT height = 1024;

  constexpr size_t y_plane_size = width * height;
  constexpr size_t uv_plane_size = (width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint16_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint16_t>(uv_plane_size, 0);

  // P010 stores 10 valid bits in the upper part of every 16-bit word; the
  // bottom 6 bits are reserved and are not guaranteed to round-trip.
  constexpr uint16_t p010_mask = 0xFFC0;
  for (auto &v : src_y_plane_values) {
    v &= p010_mask;
  }
  for (auto &v : src_uv_plane_values) {
    v &= p010_mask;
  }

  auto shared_texture =
      dx11::create_texture_2d(dx11_device, format, width, height, true);

  // Per-plane row sizes in bytes.
  const size_t y_row_size = static_cast<size_t>(width) * 2;      // W * 16-bit
  const size_t uv_row_size = static_cast<size_t>(width / 2) * 4; // W/2 * 32-bit

  upload_planar_texture(shared_texture, format, width, height,
                        as_bytes(std::span(src_y_plane_values)), y_row_size,
                        height, as_bytes(std::span(src_uv_plane_values)),
                        uv_row_size, height / 2);

  auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE, width,
      height, format);

  auto y_plane_view = dx::create_plane_view(
      l0_device, imported_image, 0, ZE_IMAGE_FORMAT_LAYOUT_16, width, height);
  auto uv_plane_view = dx::create_plane_view(l0_device, imported_image, 1,
                                             ZE_IMAGE_FORMAT_LAYOUT_16_16,
                                             width / 2, height / 2);

  std::vector<uint16_t> dst_y_plane_values(y_plane_size);
  std::vector<uint16_t> dst_uv_plane_values(uv_plane_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_y_plane_values.data(),
                                y_plane_view, nullptr);
  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_uv_plane_values.data(),
                                uv_plane_view, nullptr);
  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  // Mask out the reserved low 6 bits before comparing.
  for (auto &v : dst_y_plane_values) {
    v &= p010_mask;
  }
  for (auto &v : dst_uv_plane_values) {
    v &= p010_mask;
  }

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::destroy_ze_image(y_plane_view);
  lzt::destroy_ze_image(uv_plane_view);
  lzt::destroy_ze_image(imported_image);
  CloseHandle(texture_shared_handle);
#endif
}

} // namespace
