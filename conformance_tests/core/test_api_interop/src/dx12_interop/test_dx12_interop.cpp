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
void test_import_fence(const ComPtr<ID3D12Device> &dx12_device,
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
  test_import_fence(dx12_device, l0_device, true);
#endif
}

LZT_TEST_F(
    DX12InteroperabilityTests,
    GivenDX12SharedFenceSignaledOnDeviceWhenWaitingByL0ThenIsSuccessfullyWaitedOn) {
#ifndef __linux__
  test_import_fence(dx12_device, l0_device, false);
#endif
}

#ifndef __linux__
void test_import_memory(const ComPtr<ID3D12Device> &dx12_device,
                        ze_device_handle_t l0_device, size_t memory_size,
                        const ComPtr<ID3D12Resource> &dx12_resource,
                        void *l0_imported_memory) {
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

  auto heap = dx12::create_heap(dx12_device, memory_size, true);
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
      committed_resource.Get(), 0, staging_buffer.Get(), 0, memory_size);

  const D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = committed_resource.Get(),
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

  lzt::append_memory_copy(l0_cmd_bundle.list, l0_memory, imported_memory,
                          memory_size, nullptr, 1, &l0_after_wait_event);

  lzt::execute_and_sync_command_bundle(l0_cmd_bundle,
                                       std::numeric_limits<uint64_t>::max());

  uint32_t *ptr = reinterpret_cast<uint32_t *>(l0_memory);
  for (uint32_t i = 0; i < memory_size / sizeof(uint32_t); ++i) {
    ASSERT_EQ(ptr[i], i) << " at index = " << i;
  }

  lzt::destroy_event(l0_after_wait_event);
  lzt::destroy_event_pool(l0_event_pool);
  lzt::destroy_command_bundle(l0_cmd_bundle);
  lzt::release_external_semaphore(external_semaphore_handle);
  lzt::free_memory(l0_memory);
  lzt::free_memory(imported_memory);
  CloseHandle(fence_shared_handle);
  CloseHandle(committed_resource_shared_handle);
#endif
}

} // namespace
