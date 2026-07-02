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

#ifndef __linux__
void test_import_fence(const ComPtr<ID3D11Device5> &dx11_device5,
                       ze_device_handle_t l0_device,
                       LPCWSTR fence_name = nullptr) {
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence, fence_name);

  auto external_semaphore_handle =
      fence_name != nullptr
          ? dx::import_fence_by_name(l0_device, fence_name,
                                     ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE)
          : dx::import_fence(l0_device, fence_shared_handle,
                             ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
}
#endif

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  test_import_fence(dx11_device5, l0_device);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceWhenImportingExternalSemaphoreByNameInGlobalNamespaceThenIsSuccess) {
#ifndef __linux__
  test_import_fence(dx11_device5, l0_device,
                    L"Global\\DX11FenceNamedImportTest");
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceWhenImportingExternalSemaphoreByNameInLocalNamespaceThenIsSuccess) {
#ifndef __linux__
  test_import_fence(dx11_device5, l0_device,
                    L"Local\\DX11FenceNamedImportTest");
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceWhenImportingExternalSemaphoreByNameWithoutNamespaceThenIsSuccess) {
#ifndef __linux__
  test_import_fence(dx11_device5, l0_device, L"DX11FenceNamedImportTest");
#endif
}

#ifndef __linux__
void test_signal_fence(const ComPtr<ID3D11Device5> &dx11_device5,
                       ze_device_handle_t l0_device,
                       LPCWSTR fence_name = nullptr) {
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence, fence_name);

  auto external_semaphore_handle =
      fence_name != nullptr
          ? dx::import_fence_by_name(l0_device, fence_name,
                                     ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE)
          : dx::import_fence(l0_device, fence_shared_handle,
                             ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  EXPECT_EQ(fence->GetCompletedValue(), 0);

  uint64_t wait_value = 1;

  auto l0_cmd_bundle =
      lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
          l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          0u, 0u);

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
}
#endif

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenSignalingByL0ThenIsSuccessfullySignaled) {
#ifndef __linux__
  test_signal_fence(dx11_device5, l0_device);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceImportedByNameWhenSignalingByL0ThenIsSuccessfullySignaled) {
#ifndef __linux__
  test_signal_fence(dx11_device5, l0_device,
                    L"Local\\DX11FenceSignalByNameTest");
#endif
}

#ifndef __linux__
void test_wait_fence(const ComPtr<ID3D11Device5> &dx11_device5,
                     const ComPtr<ID3D11DeviceContext4> &dx11_device_context4,
                     ze_device_handle_t l0_device,
                     LPCWSTR fence_name = nullptr) {
  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence, fence_name);

  auto external_semaphore_handle =
      fence_name != nullptr
          ? dx::import_fence_by_name(l0_device, fence_name,
                                     ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE)
          : dx::import_fence(l0_device, fence_shared_handle,
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
      lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
          l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          0u, 0u);

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
}
#endif

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceSignaledOnDeviceWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  test_wait_fence(dx11_device5, dx11_device_context4, l0_device);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityTests,
    GivenDX11SharedFenceSignaledOnDeviceAndImportedByNameWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  test_wait_fence(dx11_device5, dx11_device_context4, l0_device,
                  L"Local\\DX11FenceWaitByNameTest");
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
    auto external_memory_props = lzt::get_external_memory_properties(l0_device);
    if (!(external_memory_props.imageImportTypes &
          ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE)) {
      GTEST_SKIP() << "Device doesn't support D3D11 texture import.";
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

  ze_image_format_layout_t get_plane_layout(DXGI_FORMAT format,
                                            uint32_t plane_index) {
    if (plane_index > 1) {
      throw std::runtime_error("Invalid plane index.");
    }

    switch (format) {
    case DXGI_FORMAT_NV12:
      return plane_index == 0 ? ZE_IMAGE_FORMAT_LAYOUT_8
                              : ZE_IMAGE_FORMAT_LAYOUT_8_8;
    case DXGI_FORMAT_P010:
      return plane_index == 0 ? ZE_IMAGE_FORMAT_LAYOUT_16
                              : ZE_IMAGE_FORMAT_LAYOUT_16_16;
    default:
      throw std::runtime_error("Unsupported format.");
    }
  }

  template <typename T>
  void import_planar_image_test(DXGI_FORMAT format, UINT width, UINT height,
                                std::span<const std::byte> plane0_bytes,
                                size_t plane0_row_size, UINT plane0_rows,
                                std::span<const std::byte> plane1_bytes,
                                size_t plane1_row_size, UINT plane1_rows,
                                std::vector<T> &dst_y_plane_values,
                                std::vector<T> &dst_uv_plane_values) {
    auto shared_texture =
        dx11::create_texture_2d(dx11_device, format, width, height, true);

    upload_planar_texture(shared_texture, format, width, height, plane0_bytes,
                          plane0_row_size, plane0_rows, plane1_bytes,
                          plane1_row_size, plane1_rows);

    auto fence = dx11::create_fence(dx11_device5, false);
    dx11_device_context4->Signal(fence.Get(), 1);
    dx11_device_context4->Flush();
    dx11::wait_for_fence(fence, 1);

    auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
    auto imported_image = dx::import_image(
        texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE,
        width, height, format);

    auto y_plane_view =
        dx::create_plane_view(l0_device, imported_image, 0,
                              get_plane_layout(format, 0), width, height);
    auto uv_plane_view = dx::create_plane_view(l0_device, imported_image, 1,
                                               get_plane_layout(format, 1),
                                               width / 2, height / 2);

    auto l0_cmd_bundle =
        lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
            l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
            ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
            ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0u, 0u);

    lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_y_plane_values.data(),
                                  y_plane_view, nullptr);
    lzt::append_image_copy_to_mem(
        l0_cmd_bundle.list, dst_uv_plane_values.data(), uv_plane_view, nullptr);
    lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                         std::numeric_limits<uint64_t>::max());

    lzt::destroy_command_bundle(l0_cmd_bundle);
    lzt::destroy_ze_image(y_plane_view);
    lzt::destroy_ze_image(uv_plane_view);
    lzt::destroy_ze_image(imported_image);
    CloseHandle(texture_shared_handle);
  }

  template <typename T>
  void import_planar_image_with_semaphore_test(
      DXGI_FORMAT format, UINT width, UINT height,
      std::span<const std::byte> plane0_bytes, size_t plane0_row_size,
      UINT plane0_rows, std::span<const std::byte> plane1_bytes,
      size_t plane1_row_size, UINT plane1_rows,
      std::vector<T> &dst_y_plane_values, std::vector<T> &dst_uv_plane_values) {
    auto fence = dx11::create_fence(dx11_device5, true);
    auto fence_shared_handle = dx11::create_shared_handle(fence);

    auto external_semaphore_handle =
        dx::import_fence(l0_device, fence_shared_handle,
                         ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

    auto l0_event_pool = lzt::create_event_pool(
        lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
    auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

    auto shared_texture =
        dx11::create_texture_2d(dx11_device, format, width, height, true);

    upload_planar_texture(shared_texture, format, width, height, plane0_bytes,
                          plane0_row_size, plane0_rows, plane1_bytes,
                          plane1_row_size, plane1_rows);

    uint64_t wait_value = 1;

    dx11_device_context4->Signal(fence.Get(), wait_value);
    dx11_device_context4->Flush();

    auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
    auto imported_image = dx::import_image(
        texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE,
        width, height, format);

    auto y_plane_view =
        dx::create_plane_view(l0_device, imported_image, 0,
                              get_plane_layout(format, 0), width, height);
    auto uv_plane_view = dx::create_plane_view(l0_device, imported_image, 1,
                                               get_plane_layout(format, 1),
                                               width / 2, height / 2);

    auto l0_cmd_bundle =
        lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
            l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
            ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
            ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0u, 0u);

    ze_external_semaphore_wait_params_ext_t semaphore_wait_params = {
        .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WAIT_PARAMS_EXT,
        .value = wait_value};
    lzt::append_wait_external_semaphore(
        l0_cmd_bundle.list, 1, &external_semaphore_handle,
        &semaphore_wait_params, l0_after_wait_event, 0, nullptr);

    lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_y_plane_values.data(),
                                  y_plane_view, nullptr, 1,
                                  &l0_after_wait_event);
    lzt::append_image_copy_to_mem(l0_cmd_bundle.list,
                                  dst_uv_plane_values.data(), uv_plane_view,
                                  nullptr, 1, &l0_after_wait_event);
    lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                         std::numeric_limits<uint64_t>::max());

    lzt::destroy_command_bundle(l0_cmd_bundle);
    lzt::destroy_ze_image(y_plane_view);
    lzt::destroy_ze_image(uv_plane_view);
    lzt::destroy_ze_image(imported_image);
    CloseHandle(texture_shared_handle);
    lzt::destroy_event(l0_after_wait_event);
    lzt::destroy_event_pool(l0_event_pool);
    lzt::release_external_semaphore(external_semaphore_handle);
    CloseHandle(fence_shared_handle);
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

  const size_t y_plane_size = width * height;
  const size_t uv_plane_size = (width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint8_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint8_t>(uv_plane_size, 0);

  std::vector<uint8_t> dst_y_plane_values(y_plane_size);
  std::vector<uint8_t> dst_uv_plane_values(uv_plane_size);

  import_planar_image_test(
      format, width, height, as_bytes(std::span(src_y_plane_values)), width,
      height, as_bytes(std::span(src_uv_plane_values)), width, height / 2,
      dst_y_plane_values, dst_uv_plane_values);

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityMultiPlanarImageTests,
    GivenDX11SharedTexture2DWithNV12FormatWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
#ifndef __linux__
  DXGI_FORMAT format = DXGI_FORMAT_NV12;
  constexpr UINT width = 1024;
  constexpr UINT height = 1024;

  const size_t y_plane_size = width * height;
  const size_t uv_plane_size = (width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint8_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint8_t>(uv_plane_size, 0);

  std::vector<uint8_t> dst_y_plane_values(y_plane_size);
  std::vector<uint8_t> dst_uv_plane_values(uv_plane_size);

  import_planar_image_with_semaphore_test(
      format, width, height, as_bytes(std::span(src_y_plane_values)), width,
      height, as_bytes(std::span(src_uv_plane_values)), width, height / 2,
      dst_y_plane_values, dst_uv_plane_values);

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);
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

  // Per-plane row sizes in bytes.
  constexpr size_t y_row_size = width * 2;        // W * 16-bit
  constexpr size_t uv_row_size = (width / 2) * 4; // W/2 * 32-bit

  std::vector<uint16_t> dst_y_plane_values(y_plane_size);
  std::vector<uint16_t> dst_uv_plane_values(uv_plane_size);

  import_planar_image_test(
      format, width, height, as_bytes(std::span(src_y_plane_values)),
      y_row_size, height, as_bytes(std::span(src_uv_plane_values)), uv_row_size,
      height / 2, dst_y_plane_values, dst_uv_plane_values);

  // Mask out the reserved low 6 bits before comparing.
  for (auto &v : dst_y_plane_values) {
    v &= p010_mask;
  }
  for (auto &v : dst_uv_plane_values) {
    v &= p010_mask;
  }

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);
#endif
}

LZT_TEST_F(
    DX11InteroperabilityMultiPlanarImageTests,
    GivenDX11SharedTexture2DWithP010FormatWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
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

  // Per-plane row sizes in bytes.
  constexpr size_t y_row_size = width * 2;        // W * 16-bit
  constexpr size_t uv_row_size = (width / 2) * 4; // W/2 * 32-bit

  std::vector<uint16_t> dst_y_plane_values(y_plane_size);
  std::vector<uint16_t> dst_uv_plane_values(uv_plane_size);

  import_planar_image_with_semaphore_test(
      format, width, height, as_bytes(std::span(src_y_plane_values)),
      y_row_size, height, as_bytes(std::span(src_uv_plane_values)), uv_row_size,
      height / 2, dst_y_plane_values, dst_uv_plane_values);

  // Mask out the reserved low 6 bits before comparing.
  for (auto &v : dst_y_plane_values) {
    v &= p010_mask;
  }
  for (auto &v : dst_uv_plane_values) {
    v &= p010_mask;
  }

  EXPECT_EQ(dst_y_plane_values, src_y_plane_values);
  EXPECT_EQ(dst_uv_plane_values, src_uv_plane_values);
#endif
}

struct DX11InteroperabilityImageTests
    : public DX11InteroperabilityMultiPlanarImageTests,
      public ::testing::WithParamInterface<int> {
#ifndef __linux__
  size_t bytes_per_pixel(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
      return 1;
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
      return 2;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_D32_FLOAT:
      return 4;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
      return 8;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return 16;
    }

    throw std::runtime_error("Unimplemented format in bytes_per_pixel.");
  }

  void upload_texture(const ComPtr<ID3D11Texture2D> &shared_texture,
                      DXGI_FORMAT format, UINT width, UINT height,
                      std::span<const std::byte> bytes, size_t row_size) {
    auto staging = dx11::create_texture_2d(dx11_device, format, width, height);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (HRESULT hr = dx11_device_context->Map(staging.Get(), 0, D3D11_MAP_WRITE,
                                              0, &mapped);
        FAILED(hr)) {
      LOG_ERROR << "ID3D11DeviceContext::Map failed with: "
                << dx::hr_to_string(hr);
      throw std::runtime_error("Failed to Map staging texture.");
    }
    for (UINT row = 0; row < height; ++row) {
      std::memcpy(static_cast<uint8_t *>(mapped.pData) + mapped.RowPitch * row,
                  bytes.data() + row_size * row, row_size);
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
    if (FAILED(keyed_mutex->ReleaseSync(0))) {
      throw std::runtime_error("ReleaseSync(0) failed on shared texture.");
    }
  }
#endif
};

LZT_TEST_P(DX11InteroperabilityImageTests,
           GivenDX11SharedTexture2DWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  DXGI_FORMAT format = static_cast<DXGI_FORMAT>(GetParam());
  constexpr UINT width = 1024;
  constexpr UINT height = 1024;

  const size_t row_size = static_cast<size_t>(width) * bytes_per_pixel(format);
  const size_t total_size = row_size * height;

  auto src_image_values = lzt::generate_vector<uint8_t>(total_size, 0);

  auto shared_texture =
      dx11::create_texture_2d(dx11_device, format, width, height, true);
  upload_texture(shared_texture, format, width, height,
                 std::as_bytes(std::span(src_image_values)), row_size);

  auto wait_fence = dx11::create_fence(dx11_device5, false);
  dx11_device_context4->Signal(wait_fence.Get(), 1);
  dx11_device_context4->Flush();
  dx11::wait_for_fence(wait_fence, 1);

  auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE, width,
      height, format);

  std::vector<uint8_t> dst_image_values(total_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
          l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          0u, 0u);

  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_image_values.data(),
                                imported_image, nullptr);
  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(dst_image_values, src_image_values);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::destroy_ze_image(imported_image);
  CloseHandle(texture_shared_handle);
#endif
}

LZT_TEST_P(
    DX11InteroperabilityImageTests,
    GivenDX11SharedTexture2DWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
#ifndef __linux__
  DXGI_FORMAT format = static_cast<DXGI_FORMAT>(GetParam());
  constexpr UINT width = 1024;
  constexpr UINT height = 1024;

  const size_t row_size = static_cast<size_t>(width) * bytes_per_pixel(format);
  const size_t total_size = row_size * height;

  auto src_image_values = lzt::generate_vector<uint8_t>(total_size, 0);

  auto shared_texture =
      dx11::create_texture_2d(dx11_device, format, width, height, true);

  auto fence = dx11::create_fence(dx11_device5, true);
  auto fence_shared_handle = dx11::create_shared_handle(fence);
  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE);

  auto l0_event_pool = lzt::create_event_pool(lzt::get_default_context(), 1,
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

  upload_texture(shared_texture, format, width, height,
                 std::as_bytes(std::span(src_image_values)), row_size);

  uint64_t wait_value = 1;

  dx11_device_context4->Signal(fence.Get(), wait_value);
  dx11_device_context4->Flush();

  auto texture_shared_handle = dx11::create_shared_handle(shared_texture);
  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE, width,
      height, format);

  std::vector<uint8_t> dst_image_values(total_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle<lzt::command_list_mode_t::immediate>(
          l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          0u, 0u);

  ze_external_semaphore_wait_params_ext_t semaphore_wait_params = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WAIT_PARAMS_EXT,
      .value = wait_value};
  lzt::append_wait_external_semaphore(
      l0_cmd_bundle.list, 1, &external_semaphore_handle, &semaphore_wait_params,
      l0_after_wait_event, 0, nullptr);

  lzt::append_image_copy_to_mem(l0_cmd_bundle.list, dst_image_values.data(),
                                imported_image, nullptr, 1,
                                &l0_after_wait_event);
  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  EXPECT_EQ(dst_image_values, src_image_values);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::destroy_event_pool(l0_event_pool);
  lzt::destroy_event(l0_after_wait_event);
  lzt::release_external_semaphore(external_semaphore_handle);
  lzt::destroy_ze_image(imported_image);
  CloseHandle(fence_shared_handle);
  CloseHandle(texture_shared_handle);
#endif
}

#ifndef __linux__
INSTANTIATE_TEST_SUITE_P(
    BasicFormats, DX11InteroperabilityImageTests,
    ::testing::Values(
        DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R8_SNORM,
        DXGI_FORMAT_R8_SINT, DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UINT,
        DXGI_FORMAT_R8G8_SNORM, DXGI_FORMAT_R8G8_SINT,
        DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UINT,
        DXGI_FORMAT_R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SINT,
        DXGI_FORMAT_R16_FLOAT, DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UINT,
        DXGI_FORMAT_R16_SNORM, DXGI_FORMAT_R16_SINT, DXGI_FORMAT_R16G16_FLOAT,
        DXGI_FORMAT_R16G16_UNORM, DXGI_FORMAT_R16G16_UINT,
        DXGI_FORMAT_R16G16_SNORM, DXGI_FORMAT_R16G16_SINT,
        DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_UNORM,
        DXGI_FORMAT_R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_SNORM,
        DXGI_FORMAT_R16G16B16A16_SINT, DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_SINT,
        DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_UINT,
        DXGI_FORMAT_R32G32B32A32_SINT),
    DXImageTestParamPrinter());
#else
INSTANTIATE_TEST_SUITE_P(BasicFormats, DX11InteroperabilityImageTests,
                         ::testing::Values(0));
#endif

} // namespace
