/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"

#ifdef __linux__
struct DX12InteroperabilityTests : ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
#include "dx12_interop_fixture.hpp"
#endif

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  ComPtr<ID3D12Fence> fence = dx12::create_fence(dx12_device, true);

  HANDLE fence_shared_handle =
      dx12::create_shared_handle(dx12_device, fence.Get());
  const ze_external_semaphore_win32_ext_desc_t external_semaphore_win32_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC,
      .handle = fence_shared_handle};

  const ze_external_semaphore_ext_desc_t external_semaphore_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC,
      .pNext = &external_semaphore_win32_desc,
      .flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D12_FENCE};

  ze_external_semaphore_ext_handle_t external_semaphore_handle = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(zeDeviceImportExternalSemaphoreExt(
      l0_device, &external_semaphore_desc, &external_semaphore_handle));

  EXPECT_NE(external_semaphore_handle, nullptr);

  EXPECT_ZE_RESULT_SUCCESS(
      zeDeviceReleaseExternalSemaphoreExt(external_semaphore_handle));

  CloseHandle(fence_shared_handle);
#endif
}

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedHeapWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  constexpr size_t memory_size = 1024;

  ComPtr<ID3D12Heap> heap = dx12::create_heap(dx12_device, memory_size, true);

  HANDLE heap_shared_handle =
      dx12::create_shared_handle(dx12_device, heap.Get());

  ze_external_memory_import_win32_handle_t import_win32_handle = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32,
      .flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP,
      .handle = heap_shared_handle};

  void *imported_memory =
      lzt::allocate_device_memory(memory_size, 0, 0, &import_win32_handle, 0,
                                  l0_device, lzt::get_default_context());

  ComPtr<ID3D12Resource> placed_resource =
      dx12::create_placed_resource(dx12_device, heap, memory_size);

  ComPtr<ID3D12Resource> staging_buffer = dx12::create_committed_resource(
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
      placed_resource.Get(), 0, staging_buffer.Get(), 0, memory_size);

  const D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = placed_resource.Get(),
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

  lzt::append_memory_copy(l0_cmd_bundle.list, l0_memory, imported_memory,
                          memory_size);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  uint32_t *ptr = reinterpret_cast<uint32_t *>(l0_memory);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    ASSERT_EQ(ptr[i], i) << " at index = " << i;
  }

  lzt::free_memory(l0_memory);
  lzt::free_memory(imported_memory);
  CloseHandle(heap_shared_handle);
#endif
}

LZT_TEST_F(DX12InteroperabilityTests,
           GivenDX12SharedCommittedResourceWhenImportingThenValuesAreCorrect) {
#ifndef __linux__
  constexpr size_t memory_size = 1024;

  ComPtr<ID3D12Resource> committed_resource = dx12::create_committed_resource(
      dx12_device, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST,
      memory_size, true);

  HANDLE committed_resource_shared_handle =
      dx12::create_shared_handle(dx12_device, committed_resource.Get());
  ze_external_memory_import_win32_handle_t import_win32_handle = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32,
      .flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE,
      .handle = committed_resource_shared_handle};
  void *imported_memory =
      lzt::allocate_device_memory(memory_size, 0, 0, &import_win32_handle, 0,
                                  l0_device, lzt::get_default_context());

  ComPtr<ID3D12Resource> staging_buffer = dx12::create_committed_resource(
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
      committed_resource.Get(), 0, staging_buffer.Get(), 0, memory_size);

  const D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = committed_resource.Get(),
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

  lzt::append_memory_copy(l0_cmd_bundle.list, l0_memory, imported_memory,
                          memory_size);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  uint32_t *ptr = reinterpret_cast<uint32_t *>(l0_memory);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    ASSERT_EQ(ptr[i], i) << " at index = " << i;
  }

  lzt::free_memory(l0_memory);
  lzt::free_memory(imported_memory);
  CloseHandle(committed_resource_shared_handle);
#endif
}
