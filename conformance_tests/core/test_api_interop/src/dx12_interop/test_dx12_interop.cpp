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
struct DX12InteroperabilityTests : ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
#include "dx12_interop/dx12_interop_fixture.hpp"
#endif

namespace {

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  auto fence = dx12::create_fence(dx12_device, true);
  auto fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE);

  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedFenceWhenSignalingByL0ThenIsSuccessfullySignaled) {
#ifndef __linux__
  auto fence = dx12::create_fence(dx12_device, true);
  auto fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE);

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

  dx12::wait_for_fence(fence, wait_value);

  EXPECT_EQ(fence->GetCompletedValue(), wait_value);

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
#endif
}

#ifndef __linux__
void test_wait_fence(const ComPtr<ID3D12Device> &dx12_device,
                     ze_device_handle_t l0_device, bool signal_on_host) {
  auto fence = dx12::create_fence(dx12_device, true);
  auto fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE);

  auto l0_event_pool = lzt::create_event_pool(lzt::get_default_context(), 1,
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

  EXPECT_EQ(fence->GetCompletedValue(), 0);

  uint64_t wait_value = 1;

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);
  if (signal_on_host) {
    fence->Signal(wait_value);
  } else {
    dx12_cmd_bundle.cmd_queue->Signal(fence.Get(), wait_value);
  }

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
}
#endif

LZT_TEST_F(
    DX12InteroperabilityTests,
    GivenDX12SharedFenceSignaledOnHostWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  test_wait_fence(dx12_device, l0_device, true);
#endif
}

LZT_TEST_F(
    DX12InteroperabilityTests,
    GivenDX12SharedFenceSignaledOnDeviceWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  test_wait_fence(dx12_device, l0_device, false);
#endif
}

#ifndef __linux__
void test_import_memory(const ComPtr<ID3D12Device> &dx12_device,
                        ze_device_handle_t l0_device, size_t memory_size,
                        const ComPtr<ID3D12Resource> &dx12_resource,
                        void *l0_imported_memory) {
  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE,
      memory_size);
  void *mapped_ptr = nullptr;
  staging_buffer->Map(0, nullptr, &mapped_ptr);
  uint32_t *mapped_ptr_as_int = reinterpret_cast<uint32_t *>(mapped_ptr);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    mapped_ptr_as_int[i] = i;
  }
  staging_buffer->Unmap(0, nullptr);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);

  dx12_cmd_bundle.cmd_list->CopyBufferRegion(
      dx12_resource.Get(), 0, staging_buffer.Get(), 0, memory_size);

  const D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = dx12_resource.Get(),
                     .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                     .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
                     .StateAfter = D3D12_RESOURCE_STATE_COMMON}};
  dx12_cmd_bundle.cmd_list->ResourceBarrier(1, &barrier);

  dx12_cmd_bundle.cmd_list->Close();
  dx12::execute_and_sync_command_bundle(dx12_device, dx12_cmd_bundle);

  void *l0_memory = lzt::allocate_host_memory(memory_size);
  memset(l0_memory, 0, memory_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

  lzt::append_memory_copy(l0_cmd_bundle.list, l0_memory, l0_imported_memory,
                          memory_size);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  uint32_t *ptr = reinterpret_cast<uint32_t *>(l0_memory);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    ASSERT_EQ(ptr[i], i) << " at index = " << i;
  }

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::free_memory(l0_memory);
}
#endif

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedHeapWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP)) {
    GTEST_SKIP() << "Device doesn't support D3D12 heap import.";
  }

  constexpr size_t memory_size = 1024;

  const auto alloc_info =
      dx12::get_placed_resource_alloc_info(dx12_device, memory_size);

  auto heap = dx12::create_heap(dx12_device, alloc_info.SizeInBytes,
                                alloc_info.Alignment, true);
  auto heap_shared_handle = dx12::create_shared_handle(dx12_device, heap.Get());

  void *imported_memory = import_memory(
      heap_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP, memory_size);

  auto placed_resource =
      dx12::create_placed_resource(dx12_device, heap, memory_size);

  test_import_memory(dx12_device, l0_device, memory_size, placed_resource,
                     imported_memory);

  lzt::free_memory(imported_memory);
  CloseHandle(heap_shared_handle);
#endif
}

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedCommittedResourceWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 committed resource import.";
  }

  constexpr size_t memory_size = 1024;

  auto committed_resource = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST,
      memory_size, true);
  auto committed_resource_shared_handle =
      dx12::create_shared_handle(dx12_device, committed_resource.Get());

  void *imported_memory =
      import_memory(committed_resource_shared_handle,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, memory_size);

  test_import_memory(dx12_device, l0_device, memory_size, committed_resource,
                     imported_memory);

  lzt::free_memory(imported_memory);
  CloseHandle(committed_resource_shared_handle);
#endif
}

#ifndef __linux__
void test_import_memory_with_semaphore(
    const ComPtr<ID3D12Device> &dx12_device, ze_device_handle_t l0_device,
    size_t memory_size, const ComPtr<ID3D12Resource> &dx12_resource,
    void *l0_imported_memory) {
  auto fence = dx12::create_fence(dx12_device, true);
  auto fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE);

  auto l0_event_pool = lzt::create_event_pool(lzt::get_default_context(), 1,
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ,
      memory_size);
  void *mapped_ptr = nullptr;
  staging_buffer->Map(0, nullptr, &mapped_ptr);
  uint32_t *mapped_ptr_as_int = reinterpret_cast<uint32_t *>(mapped_ptr);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    mapped_ptr_as_int[i] = i;
  }
  staging_buffer->Unmap(0, nullptr);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);

  dx12_cmd_bundle.cmd_list->CopyBufferRegion(
      dx12_resource.Get(), 0, staging_buffer.Get(), 0, memory_size);

  const D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = dx12_resource.Get(),
                     .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                     .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
                     .StateAfter = D3D12_RESOURCE_STATE_COMMON}};
  dx12_cmd_bundle.cmd_list->ResourceBarrier(1, &barrier);
  dx12_cmd_bundle.cmd_list->Close();

  ID3D12CommandList *lists[] = {dx12_cmd_bundle.cmd_list.Get()};
  dx12_cmd_bundle.cmd_queue->ExecuteCommandLists(1, lists);

  uint64_t wait_value = 1;
  dx12_cmd_bundle.cmd_queue->Signal(fence.Get(), wait_value);

  void *l0_memory = lzt::allocate_host_memory(memory_size);
  memset(l0_memory, 0, memory_size);

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

  lzt::append_memory_copy(l0_cmd_bundle.list, l0_memory, l0_imported_memory,
                          memory_size, nullptr, 1, &l0_after_wait_event);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  uint32_t *ptr = reinterpret_cast<uint32_t *>(l0_memory);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    ASSERT_EQ(ptr[i], i) << " at index = " << i;
  }

  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::free_memory(l0_memory);
  lzt::destroy_event_pool(l0_event_pool);
  lzt::destroy_event(l0_after_wait_event);
  lzt::release_external_semaphore(external_semaphore_handle);
  CloseHandle(fence_shared_handle);
}
#endif

LZT_TEST_F(
    DX12InteroperabilityTests,
    GivenDX12SharedHeapWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP)) {
    GTEST_SKIP() << "Device doesn't support D3D12 heap import.";
  }

  constexpr size_t memory_size = 1024;

  const auto alloc_info =
      dx12::get_placed_resource_alloc_info(dx12_device, memory_size);

  auto heap = dx12::create_heap(dx12_device, alloc_info.SizeInBytes,
                                alloc_info.Alignment, true);
  auto heap_shared_handle = dx12::create_shared_handle(dx12_device, heap.Get());

  void *imported_memory = import_memory(
      heap_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP, memory_size);

  auto placed_resource =
      dx12::create_placed_resource(dx12_device, heap, memory_size);

  test_import_memory_with_semaphore(dx12_device, l0_device, memory_size,
                                    placed_resource, imported_memory);

  lzt::free_memory(imported_memory);
  CloseHandle(heap_shared_handle);
#endif
}

LZT_TEST_F(
    DX12InteroperabilityTests,
    GivenDX12SharedCommittedResourceWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.memoryAllocationImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 committed resource import.";
  }

  constexpr size_t memory_size = 1024;

  auto committed_resource = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST,
      memory_size, true);
  auto committed_resource_shared_handle =
      dx12::create_shared_handle(dx12_device, committed_resource.Get());

  void *imported_memory =
      import_memory(committed_resource_shared_handle,
                    ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, memory_size);

  test_import_memory_with_semaphore(dx12_device, l0_device, memory_size,
                                    committed_resource, imported_memory);

  lzt::free_memory(imported_memory);
  CloseHandle(committed_resource_shared_handle);
#endif
}

struct DX12IteroperabilityMultiPlanarImageTests
    : public DX12InteroperabilityTests {
  void SetUp() override {
#ifndef __linux__
    DX12InteroperabilityTests::SetUp();
    auto external_memory_props = lzt::get_external_memory_properties(l0_device);
    if (!lzt::image_support(l0_device)) {
      GTEST_SKIP() << "Device doesn't support images.";
    }

#else
    GTEST_SKIP() << "Not supported on Linux";
#endif
  }

#ifndef __linux__
  void record_fill_multiplanar_image(
      const ComPtr<ID3D12GraphicsCommandList> &dx12_cmd_list,
      const ComPtr<ID3D12Resource> &texture,
      const ComPtr<ID3D12Resource> &staging_buffer,
      std::span<const D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints,
      std::span<const UINT> num_rows, std::span<const size_t> row_size,
      std::span<const std::byte> y_plane_bytes,
      std::span<const std::byte> uv_plane_bytes) {
    void *mapped_ptr = nullptr;
    staging_buffer->Map(0, nullptr, &mapped_ptr);
    // Copy Y plane
    for (UINT row = 0; row < num_rows[0]; ++row) {
      std::memcpy(static_cast<uint8_t *>(mapped_ptr) + footprints[0].Offset +
                      footprints[0].Footprint.RowPitch * row,
                  y_plane_bytes.data() + row_size[0] * row, row_size[0]);
    }
    // Copy UV plane
    for (UINT row = 0; row < num_rows[1]; ++row) {
      std::memcpy(static_cast<uint8_t *>(mapped_ptr) + footprints[1].Offset +
                      footprints[1].Footprint.RowPitch * row,
                  uv_plane_bytes.data() + row_size[1] * row, row_size[1]);
    }
    staging_buffer->Unmap(0, nullptr);

    for (UINT p = 0; p < 2; ++p) {
      D3D12_TEXTURE_COPY_LOCATION dst = {
          .pResource = texture.Get(),
          .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
          .SubresourceIndex = p};
      D3D12_TEXTURE_COPY_LOCATION src = {
          .pResource = staging_buffer.Get(),
          .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
          .PlacedFootprint = footprints[p]};
      dx12_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }

    const D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Transition = {
          .pResource = texture.Get(),
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
          .StateAfter = D3D12_RESOURCE_STATE_COMMON
        }};
    dx12_cmd_list->ResourceBarrier(1, &barrier);
  }
#endif
};

LZT_TEST_F(
    DX12IteroperabilityMultiPlanarImageTests,
    GivenDX12SharedTexture2DWithNV12FormatWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.imageImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 image resource import.";
  }

  DXGI_FORMAT format = DXGI_FORMAT_NV12;
  constexpr UINT64 width = 1024;
  constexpr UINT height = 1024;

  auto texture = dx12::create_committed_texture_2d(dx12_device, format, width,
                                                   height, true);
  auto texture_shared_handle =
      dx12::create_shared_handle(dx12_device, texture.Get());

  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, width,
      height, format);

  D3D12_RESOURCE_DESC resource_desc = texture->GetDesc();
  const UINT plane_count =
      dx12::get_format_plane_count(dx12_device, resource_desc.Format);
  ASSERT_EQ(plane_count, 2) << "Unexpected plane count for NV12 format.";

  std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(plane_count);
  std::vector<UINT> num_rows(plane_count);
  std::vector<UINT64> row_size(plane_count);
  UINT64 total_size = 0;
  dx12_device->GetCopyableFootprints(&resource_desc, 0, plane_count, 0,
                                     footprints.data(), num_rows.data(),
                                     row_size.data(), &total_size);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);
  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE,
      total_size);

  const size_t y_plane_size = width * height;
  const size_t uv_plane_size = (width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint8_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint8_t>(uv_plane_size, 0);

  record_fill_multiplanar_image(dx12_cmd_bundle.cmd_list, texture,
                                staging_buffer, footprints, num_rows, row_size,
                                as_bytes(std::span(src_y_plane_values)),
                                as_bytes(std::span(src_uv_plane_values)));

  dx12_cmd_bundle.cmd_list->Close();
  dx12::execute_and_sync_command_bundle(dx12_device, dx12_cmd_bundle);

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
    DX12IteroperabilityMultiPlanarImageTests,
    GivenDX12SharedTexture2DWithP010FormatWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.imageImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 image resource import.";
  }

  DXGI_FORMAT format = DXGI_FORMAT_P010;
  constexpr UINT64 width = 1024;
  constexpr UINT height = 1024;

  auto texture = dx12::create_committed_texture_2d(dx12_device, format, width,
                                                   height, true);
  auto texture_shared_handle =
      dx12::create_shared_handle(dx12_device, texture.Get());

  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, width,
      height, format);

  D3D12_RESOURCE_DESC resource_desc = texture->GetDesc();
  const UINT plane_count =
      dx12::get_format_plane_count(dx12_device, resource_desc.Format);
  ASSERT_EQ(plane_count, 2) << "Unexpected plane count for P010 format.";

  std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(plane_count);
  std::vector<UINT> num_rows(plane_count);
  std::vector<UINT64> row_size(plane_count);
  UINT64 total_size = 0;
  dx12_device->GetCopyableFootprints(&resource_desc, 0, plane_count, 0,
                                     footprints.data(), num_rows.data(),
                                     row_size.data(), &total_size);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);
  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE,
      total_size);

  constexpr size_t y_plane_size = width * height;
  constexpr size_t uv_plane_size = (width / 2) * (height / 2) * 2;
  auto src_y_plane_values = lzt::generate_vector<uint16_t>(y_plane_size, 0);
  auto src_uv_plane_values = lzt::generate_vector<uint16_t>(uv_plane_size, 0);

  // P010 stores 10 valid bits in the upper part of every 16-bit word; the
  // bottom 6 bits are reserved and are not guaranteed to round-trip through
  // the GPU. Mask the source so it only contains valid data.
  constexpr uint16_t p010_mask = 0xFFC0;
  for (auto &v : src_y_plane_values) {
    v &= p010_mask;
  }
  for (auto &v : src_uv_plane_values) {
    v &= p010_mask;
  }

  record_fill_multiplanar_image(dx12_cmd_bundle.cmd_list, texture,
                                staging_buffer, footprints, num_rows, row_size,
                                std::as_bytes(std::span(src_y_plane_values)),
                                std::as_bytes(std::span(src_uv_plane_values)));

  dx12_cmd_bundle.cmd_list->Close();
  dx12::execute_and_sync_command_bundle(dx12_device, dx12_cmd_bundle);

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

  // Mask out the reserved low 6 bits so the comparison only checks the
  // valid 10-bit P010 data
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

struct DX12InteroperabilityImageTests
    : public DX12IteroperabilityMultiPlanarImageTests,
      public ::testing::WithParamInterface<int> {
#ifndef __linux__
  void record_fill_image(const ComPtr<ID3D12GraphicsCommandList> &dx12_cmd_list,
                         const ComPtr<ID3D12Resource> &texture,
                         const ComPtr<ID3D12Resource> &staging_buffer,
                         const D3D12_PLACED_SUBRESOURCE_FOOTPRINT &footprint,
                         UINT num_rows, size_t row_size,
                         std::span<const uint8_t> bytes) {
    void *mapped_ptr = nullptr;
    staging_buffer->Map(0, nullptr, &mapped_ptr);
    for (UINT row = 0; row < num_rows; ++row) {
      memcpy(static_cast<uint8_t *>(mapped_ptr) + footprint.Offset +
                 static_cast<size_t>(footprint.Footprint.RowPitch) * row,
             bytes.data() + static_cast<size_t>(row_size) * row,
             static_cast<size_t>(row_size));
    }
    staging_buffer->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst = {
        .pResource = texture.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
        .SubresourceIndex = 0};
    D3D12_TEXTURE_COPY_LOCATION src = {
        .pResource = staging_buffer.Get(),
        .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
        .PlacedFootprint = footprint};
    dx12_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    const D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, .Transition = {
          .pResource = texture.Get(),
          .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
          .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
          .StateAfter = D3D12_RESOURCE_STATE_COMMON
        }};
    dx12_cmd_list->ResourceBarrier(1, &barrier);
  }
#endif
};

LZT_TEST_P(DX12InteroperabilityImageTests,
           GivenDX12SharedTexture2DWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.imageImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 image resource import.";
  }

  DXGI_FORMAT format = static_cast<DXGI_FORMAT>(GetParam());
  constexpr UINT64 width = 1024;
  constexpr UINT height = 1024;

  auto texture = dx12::create_committed_texture_2d(dx12_device, format, width,
                                                   height, true);
  auto texture_shared_handle =
      dx12::create_shared_handle(dx12_device, texture.Get());

  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, width,
      height, format);

  D3D12_RESOURCE_DESC resource_desc = texture->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT num_rows = 0;
  UINT64 row_size = 0;
  UINT64 total_size = 0;
  dx12_device->GetCopyableFootprints(&resource_desc, 0, 1, 0, &footprint,
                                     &num_rows, &row_size, &total_size);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);
  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE,
      total_size);

  auto src_image_values = lzt::generate_vector<uint8_t>(total_size, 0);
  record_fill_image(dx12_cmd_bundle.cmd_list, texture, staging_buffer,
                    footprint, num_rows, row_size, src_image_values);

  dx12_cmd_bundle.cmd_list->Close();
  dx12::execute_and_sync_command_bundle(dx12_device, dx12_cmd_bundle);

  std::vector<uint8_t> dst_image_values(total_size);

  auto l0_cmd_bundle =
      lzt::create_command_bundle(l0_device, ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                 ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                 ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, true);

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
    DX12InteroperabilityImageTests,
    GivenDX12SharedTexture2DWhenSynchronizingWithExternalSemaphoreThenValuesAreCorrect) {
#ifndef __linux__
  auto external_memory_props = lzt::get_external_memory_properties(l0_device);
  if (!(external_memory_props.imageImportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE)) {
    GTEST_SKIP() << "Device doesn't support D3D12 image resource import.";
  }

  DXGI_FORMAT format = static_cast<DXGI_FORMAT>(GetParam());
  constexpr UINT64 width = 1024;
  constexpr UINT height = 1024;

  auto texture = dx12::create_committed_texture_2d(dx12_device, format, width,
                                                   height, true);
  auto texture_shared_handle =
      dx12::create_shared_handle(dx12_device, texture.Get());

  auto imported_image = dx::import_image(
      texture_shared_handle, ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE, width,
      height, format);

  auto fence = dx12::create_fence(dx12_device, true);
  auto fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());

  auto external_semaphore_handle =
      dx::import_fence(l0_device, fence_shared_handle,
                       ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE);

  auto l0_event_pool = lzt::create_event_pool(lzt::get_default_context(), 1,
                                              ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  ze_event_desc_t l0_event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  auto l0_after_wait_event = lzt::create_event(l0_event_pool, l0_event_desc);

  D3D12_RESOURCE_DESC resource_desc = texture->GetDesc();
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
  UINT num_rows = 0;
  UINT64 row_size = 0;
  UINT64 total_size = 0;
  dx12_device->GetCopyableFootprints(&resource_desc, 0, 1, 0, &footprint,
                                     &num_rows, &row_size, &total_size);

  auto dx12_cmd_bundle = dx12::create_command_bundle(dx12_device);
  auto staging_buffer = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_COPY_SOURCE,
      total_size);

  auto src_image_values = lzt::generate_vector<uint8_t>(total_size, 0);
  record_fill_image(dx12_cmd_bundle.cmd_list, texture, staging_buffer,
                    footprint, num_rows, row_size, src_image_values);

  dx12_cmd_bundle.cmd_list->Close();

  ID3D12CommandList *lists[] = {dx12_cmd_bundle.cmd_list.Get()};
  dx12_cmd_bundle.cmd_queue->ExecuteCommandLists(1, lists);

  uint64_t wait_value = 1;
  dx12_cmd_bundle.cmd_queue->Signal(fence.Get(), wait_value);

  std::vector<uint8_t> dst_image_values(total_size);

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
  CloseHandle(fence_shared_handle);
  lzt::destroy_ze_image(imported_image);
  CloseHandle(texture_shared_handle);
#endif
}

#ifndef __linux__
struct DX12ImageTestParamPrinter {
  template <class ParamType>
  std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
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
#endif

#ifndef __linux__
INSTANTIATE_TEST_SUITE_P(
    BasicFormats, DX12InteroperabilityImageTests,
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
        DXGI_FORMAT_R32_UINT, DXGI_FORMAT_R32_SINT, DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32G32_FLOAT, DXGI_FORMAT_R32G32_UINT,
        DXGI_FORMAT_R32G32_SINT, DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_SINT),
    DX12ImageTestParamPrinter());
#else
INSTANTIATE_TEST_SUITE_P(BasicFormats, DX12InteroperabilityImageTests,
                         ::testing::Values(0));
#endif

} // namespace
