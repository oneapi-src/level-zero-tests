/*
 *
 * Copyright (C) 2019 - 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace level_zero_tests {

void *aligned_malloc(size_t size, size_t alignment) {

  EXPECT_NE(0u, size);
  EXPECT_FALSE(alignment != 0 && (alignment & (alignment - 1)) != 0);

  void *memory = nullptr;

#ifdef __linux__
  if (size % alignment != 0) {
    size = ((size + alignment - 1) / alignment) * alignment;
  }
  memory = aligned_alloc(alignment, size);
#else // Windows
  memory = _aligned_malloc(size, alignment);
#endif

  EXPECT_NE(nullptr, memory);
  return memory;
}

void *aligned_malloc_no_check(size_t size, size_t alignment,
                              ze_result_t *result) {
  if (size == 0) {
    *result = ZE_RESULT_ERROR_UNSUPPORTED_SIZE;
    return nullptr;
  }

  if (alignment != 0 && (alignment & (alignment - 1)) != 0) {
    *result = ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT;
    return nullptr;
  }

  void *memory = nullptr;

#ifdef __linux__
  if (size % alignment != 0) {
    size = ((size + alignment - 1) / alignment) * alignment;
  }
  memory = aligned_alloc(alignment, size);
#else // Windows
  memory = _aligned_malloc(size, alignment);
#endif

  if (!memory) {
    *result = ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  }
  return memory;
}

void aligned_free(void *ptr) {
#ifdef __linux__
  free(ptr);
#else // Windows
  _aligned_free((void *)ptr);
#endif
}

void *allocate_host_memory(const size_t size) {
  return allocate_host_memory(size, 1);
}

void *allocate_host_memory_with_allocator_selector(const size_t size,
                                                   bool is_shared_system) {
  if (is_shared_system) {
    return aligned_malloc(size, 1);
  }
  return allocate_host_memory(size, 1);
}

void *allocate_host_memory(const size_t size, const size_t alignment) {
  auto context = lzt::get_default_context();

  return allocate_host_memory(size, alignment, context);
}

void *allocate_host_memory_no_check(const size_t size, const size_t alignment,
                                    ze_context_handle_t context,
                                    ze_result_t *result) {

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.flags = 0;

  host_desc.pNext = nullptr;

  void *memory = nullptr;
  *result = zeMemAllocHost(context, &host_desc, size, alignment, &memory);

  return memory;
}

void *allocate_host_memory(const size_t size, const size_t alignment,
                           ze_context_handle_t context) {

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.flags = 0;

  host_desc.pNext = nullptr;

  void *memory = nullptr;
  auto context_initial = context;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(context, &host_desc, size, alignment, &memory));
  EXPECT_EQ(context, context_initial);
  EXPECT_NE(nullptr, memory);

  return memory;
}

void *allocate_host_memory(const size_t size, const size_t alignment,
                           const ze_host_mem_alloc_flags_t flags, void *pNext,
                           ze_context_handle_t context) {

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.flags = flags;
  host_desc.pNext = pNext;

  void *memory = nullptr;
  auto context_initial = context;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(context, &host_desc, size, alignment, &memory));
  EXPECT_EQ(context, context_initial);
  EXPECT_NE(nullptr, memory);

  return memory;
}

void *allocate_device_memory(const size_t size) {
  return allocate_device_memory(size, 1);
}

void *allocate_device_memory(const size_t size, const size_t alignment) {
  return allocate_device_memory(size, alignment, 0);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t flags) {

  auto context = lzt::get_default_context();
  auto device = zeDevice::get_instance()->get_device();
  return allocate_device_memory(size, alignment, flags, device, context);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t flags,
                             ze_context_handle_t context) {

  auto device = zeDevice::get_instance()->get_device();
  return allocate_device_memory(size, alignment, flags, device, context);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t flags,
                             ze_device_handle_t device,
                             ze_context_handle_t context) {

  return allocate_device_memory(size, alignment, flags, 0, device, context);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t flags,
                             const uint32_t ordinal,
                             ze_device_handle_t device_handle,
                             ze_context_handle_t context) {

  return lzt::allocate_device_memory(size, alignment, flags, nullptr, ordinal,
                                     device_handle, context);
}

void *allocate_device_memory_no_check(const size_t size, const size_t alignment,
                                      const ze_device_mem_alloc_flags_t flags,
                                      void *pNext, const uint32_t ordinal,
                                      ze_device_handle_t device_handle,
                                      ze_context_handle_t context,
                                      ze_result_t *result) {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = ordinal;
  device_desc.flags = flags;
  device_desc.pNext = pNext;

  void *memory = nullptr;
  *result = zeMemAllocDevice(context, &device_desc, size, alignment,
                             device_handle, &memory);

  return memory;
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t flags,
                             void *pNext, const uint32_t ordinal,
                             ze_device_handle_t device_handle,
                             ze_context_handle_t context) {
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = ordinal;
  device_desc.flags = flags;
  device_desc.pNext = pNext;

  auto context_initial = context;
  auto device_initial = device_handle;

  void *memory = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocDevice(context, &device_desc, size, alignment,
                             device_handle, &memory));
  EXPECT_EQ(context, context_initial);
  EXPECT_EQ(device_handle, device_initial);
  EXPECT_NE(nullptr, memory);

  return memory;
}

void *allocate_shared_memory(const size_t size) {
  return allocate_shared_memory(size, 1);
}

void *allocate_shared_memory_with_allocator_selector(const size_t size,
                                                     bool is_shared_system) {
  if (is_shared_system) {
    return aligned_malloc(size, 1);
  }
  return allocate_shared_memory(size, 1);
}

void *allocate_shared_memory(const size_t size, ze_device_handle_t device) {
  return allocate_shared_memory(size, 1, 0, 0, device);
}

void *allocate_shared_memory(const size_t size, const size_t alignment) {
  return allocate_shared_memory(size, alignment, 0, 0);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             ze_context_handle_t context) {
  ze_device_handle_t device = zeDevice::get_instance()->get_device();
  return allocate_shared_memory(size, alignment, 0, 0, device, context);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t dev_flags,
                             const ze_host_mem_alloc_flags_t host_flags) {

  ze_device_handle_t device = zeDevice::get_instance()->get_device();

  return allocate_shared_memory(size, alignment, dev_flags, host_flags, device);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t dev_flags,
                             const ze_host_mem_alloc_flags_t host_flags,
                             ze_device_handle_t device) {

  auto context = lzt::get_default_context();
  return allocate_shared_memory(size, alignment, dev_flags, host_flags, device,
                                context);
}

void *allocate_shared_memory_with_allocator_selector(
    const size_t size, const size_t alignment,
    const ze_device_mem_alloc_flags_t dev_flags,
    const ze_host_mem_alloc_flags_t host_flags, ze_device_handle_t device,
    bool is_shared_system) {

  if (is_shared_system) {
    return aligned_malloc(size, alignment);
  }

  auto context = lzt::get_default_context();
  return allocate_shared_memory(size, alignment, dev_flags, host_flags, device,
                                context);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t dev_flags,
                             const ze_host_mem_alloc_flags_t host_flags,
                             ze_device_handle_t device,
                             ze_context_handle_t context) {

  return allocate_shared_memory(size, alignment, dev_flags, nullptr, host_flags,
                                nullptr, device, context);
}

void *allocate_shared_memory_with_allocator_selector(
    const size_t size, const size_t alignment,
    const ze_device_mem_alloc_flags_t dev_flags,
    const ze_host_mem_alloc_flags_t host_flags, ze_device_handle_t device,
    ze_context_handle_t context, bool is_shared_system) {

  if (is_shared_system) {
    return aligned_malloc(size, alignment);
  }

  return allocate_shared_memory(size, alignment, dev_flags, nullptr, host_flags,
                                nullptr, device, context);
}

void *allocate_shared_memory_no_check(
    const size_t size, const size_t alignment,
    const ze_device_mem_alloc_flags_t device_flags, void *device_pNext,
    const ze_host_mem_alloc_flags_t host_flags, void *host_pNext,
    ze_device_handle_t device, ze_context_handle_t context,
    ze_result_t *result) {

  uint32_t ordinal = 0;
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = ordinal;
  device_desc.flags = device_flags;
  device_desc.pNext = device_pNext;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.flags = host_flags;
  host_desc.pNext = host_pNext;

  void *memory = nullptr;
  *result = zeMemAllocShared(context, &device_desc, &host_desc, size, alignment,
                             device, &memory);

  return memory;
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flags_t device_flags,
                             void *device_pNext,
                             const ze_host_mem_alloc_flags_t host_flags,
                             void *host_pNext, ze_device_handle_t device,
                             ze_context_handle_t context) {

  uint32_t ordinal = 0;
  ze_device_mem_alloc_desc_t device_desc = {};
  device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  device_desc.ordinal = ordinal;
  device_desc.flags = device_flags;
  device_desc.pNext = device_pNext;

  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;
  host_desc.flags = host_flags;
  host_desc.pNext = host_pNext;

  auto context_initial = context;
  auto device_initial = device;

  void *memory = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocShared(context, &device_desc, &host_desc, size, alignment,
                             device, &memory));
  EXPECT_EQ(context, context_initial);
  EXPECT_EQ(device, device_initial);
  EXPECT_NE(nullptr, memory);

  return memory;
}

void allocate_mem(void **memory, ze_memory_type_t mem_type, size_t size,
                  ze_context_handle_t context) {
  switch (mem_type) {
  case ZE_MEMORY_TYPE_HOST:
    *memory = allocate_host_memory(size, 1, context);
    break;
  case ZE_MEMORY_TYPE_DEVICE: {
    auto device = zeDevice::get_instance()->get_device();
    *memory = allocate_device_memory(size, 1, 0, 0, device, context);
    break;
  }
  case ZE_MEMORY_TYPE_SHARED:
    *memory = allocate_shared_memory(size, 1, context);
    break;
  case ZE_MEMORY_TYPE_UNKNOWN:
  default:
    break;
  }
}

void free_memory(const void *ptr) {
  free_memory(lzt::get_default_context(), ptr);
}

void free_memory_with_allocator_selector(const void *ptr,
                                         bool is_shared_system) {
  if (is_shared_system) {
    aligned_free((void *)ptr);
  } else {
    free_memory(lzt::get_default_context(), ptr);
  }
}

void free_memory(ze_context_handle_t context, const void *ptr) {
  auto context_initial = context;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, (void *)ptr));
  EXPECT_EQ(context, context_initial);
}

void free_memory_with_allocator_selector(ze_context_handle_t context,
                                         const void *ptr,
                                         bool is_shared_system) {
  if (is_shared_system) {
    aligned_free((void *)ptr);
  } else {
    auto context_initial = context;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemFree(context, (void *)ptr));
    EXPECT_EQ(context, context_initial);
  }
}

void allocate_mem_and_get_ipc_handle(ze_context_handle_t context,
                                     ze_ipc_mem_handle_t *mem_handle,
                                     void **memory, ze_memory_type_t mem_type) {
  allocate_mem_and_get_ipc_handle(context, mem_handle, memory, mem_type, 1);
}

void allocate_mem_and_get_ipc_handle(ze_context_handle_t context,
                                     ze_ipc_mem_handle_t *mem_handle,
                                     void **memory, ze_memory_type_t mem_type,
                                     size_t size) {
  allocate_mem(memory, mem_type, size, context);
  get_ipc_handle(context, mem_handle, *memory);
}

void get_ipc_handle(ze_context_handle_t context,
                    ze_ipc_mem_handle_t *mem_handle, void *memory) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetIpcHandle(context, memory, mem_handle));
}

void put_ipc_handle(ze_context_handle_t context,
                    ze_ipc_mem_handle_t mem_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context, mem_handle));
}

void open_ipc_handle(ze_context_handle_t context, ze_device_handle_t device,
                     ze_ipc_mem_handle_t mem_handle, void **memory) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, mem_handle, 0, memory));
}

void close_ipc_handle(ze_context_handle_t context, void **memory) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
}

void write_data_pattern(void *buff, size_t size, int8_t data_pattern) {
  int8_t *pbuff = static_cast<int8_t *>(buff);
  int8_t dp = data_pattern;
  for (size_t i = 0; i < size; i++) {
    pbuff[i] = dp;
    dp = (dp + data_pattern) & 0xff;
  }
}

void validate_data_pattern(void *buff, size_t size, int8_t data_pattern) {
  int8_t *pbuff = static_cast<int8_t *>(buff);
  int8_t dp = data_pattern;
  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(pbuff[i], dp);
    dp = (dp + data_pattern) & 0xff;
  }
}
void get_mem_alloc_properties(
    ze_context_handle_t context, const void *memory,
    ze_memory_allocation_properties_t *memory_properties) {
  get_mem_alloc_properties(context, memory, memory_properties, nullptr);
}
void get_mem_alloc_properties(
    ze_context_handle_t context, const void *memory,
    ze_memory_allocation_properties_t *memory_properties,
    ze_device_handle_t *device) {
  auto context_initial = context;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAllocProperties(
                                   context, memory, memory_properties, device));
  EXPECT_EQ(context, context_initial);
}

void *reserve_allocate_and_map_device_memory(
    ze_context_handle_t context, ze_device_handle_t device, size_t &allocSize,
    ze_physical_mem_handle_t *reservedPhysicalMemory) {
  size_t pageSize = 0;
  void *reservedMemory = nullptr;
  lzt::query_page_size(context, device, allocSize, &pageSize);
  allocSize = lzt::create_page_aligned_size(allocSize, pageSize);
  lzt::physical_device_memory_allocation(context, device, allocSize,
                                         reservedPhysicalMemory);
  lzt::virtual_memory_reservation(context, nullptr, allocSize, &reservedMemory);
  EXPECT_NE(nullptr, reservedMemory);
  EXPECT_EQ(zeVirtualMemMap(context, reservedMemory, allocSize,
                            *reservedPhysicalMemory, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
            ZE_RESULT_SUCCESS);
  return reservedMemory;
}

void unmap_and_free_reserved_memory(
    ze_context_handle_t context, void *reservedMemory,
    ze_physical_mem_handle_t reservedPhysicalMemory, size_t allocSize) {
  lzt::virtual_memory_unmap(context, reservedMemory, allocSize);
  lzt::physical_memory_destroy(context, reservedPhysicalMemory);
  lzt::virtual_memory_free(context, reservedMemory, allocSize);
}

void query_page_size(ze_context_handle_t context, ze_device_handle_t device,
                     size_t alloc_size, size_t *page_size) {
  EXPECT_EQ(zeVirtualMemQueryPageSize(context, device, alloc_size, page_size),
            ZE_RESULT_SUCCESS);
  EXPECT_GT(*page_size, 0);
}

void virtual_memory_reservation(ze_context_handle_t context, const void *pStart,
                                size_t size, void **memory) {
  ASSERT_EQ(zeVirtualMemReserve(context, pStart, size, memory),
            ZE_RESULT_SUCCESS);
}

void virtual_memory_reservation_set_access(
    ze_context_handle_t context, const void *ptr, size_t size,
    ze_memory_access_attribute_t access) {
  EXPECT_EQ(zeVirtualMemSetAccessAttribute(context, ptr, size, access),
            ZE_RESULT_SUCCESS);
}

void virtual_memory_reservation_get_access(ze_context_handle_t context,
                                           const void *ptr, size_t size,
                                           ze_memory_access_attribute_t *access,
                                           size_t *outSize) {
  EXPECT_EQ(zeVirtualMemGetAccessAttribute(context, ptr, size, access, outSize),
            ZE_RESULT_SUCCESS);
}

void virtual_memory_free(ze_context_handle_t context, void *memory,
                         size_t alloc_size) {
  EXPECT_EQ(zeVirtualMemFree(context, memory, alloc_size), ZE_RESULT_SUCCESS);
}

void physical_device_memory_allocation(ze_context_handle_t context,
                                       ze_device_handle_t device,
                                       size_t allocation_size,
                                       ze_physical_mem_handle_t *memory) {
  ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC,
                                     nullptr};
  physDesc.size = allocation_size;
  physDesc.flags = ZE_PHYSICAL_MEM_FLAG_ALLOCATE_ON_DEVICE;
  lzt::physical_memory_allocation(context, device, &physDesc, memory);
}

void physical_host_memory_allocation(ze_context_handle_t context,
                                     size_t allocation_size,
                                     ze_physical_mem_handle_t *memory) {
  ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC,
                                     nullptr};
  physDesc.size = allocation_size;
  physDesc.flags = ZE_PHYSICAL_MEM_FLAG_ALLOCATE_ON_HOST;
  lzt::physical_memory_allocation(context, nullptr, &physDesc, memory);
}

void physical_memory_allocation(ze_context_handle_t context,
                                ze_device_handle_t device,
                                ze_physical_mem_desc_t *desc,
                                ze_physical_mem_handle_t *memory) {
  EXPECT_EQ(zePhysicalMemCreate(context, device, desc, memory),
            ZE_RESULT_SUCCESS);
  EXPECT_NE(nullptr, memory);
}

void physical_memory_destroy(ze_context_handle_t context,
                             ze_physical_mem_handle_t memory) {
  EXPECT_EQ(zePhysicalMemDestroy(context, memory), ZE_RESULT_SUCCESS);
}

void virtual_memory_map(ze_context_handle_t context,
                        const void *reservedVirtualMemory, size_t size,
                        ze_physical_mem_handle_t physical_memory, size_t offset,
                        ze_memory_access_attribute_t access) {
  EXPECT_EQ(zeVirtualMemMap(context, reservedVirtualMemory, size,
                            physical_memory, offset, access),
            ZE_RESULT_SUCCESS);
}

void virtual_memory_unmap(ze_context_handle_t context,
                          const void *reservedVirtualMemory, size_t size) {
  EXPECT_EQ(zeVirtualMemUnmap(context, reservedVirtualMemory, size),
            ZE_RESULT_SUCCESS);
}

size_t create_page_aligned_size(size_t requested_size, size_t page_size) {
  if (page_size >= requested_size) {
    return page_size;
  } else {
    return (requested_size / page_size) * page_size;
  }
}

void gtest_skip_if_shared_system_alloc_unsupported(bool is_shared_system) {
  if (is_shared_system) {
    const auto memory_access_properties = lzt::get_memory_access_properties(
        lzt::get_default_device(lzt::get_default_driver()));

    if ((memory_access_properties.sharedSystemAllocCapabilities &
         ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
      GTEST_SKIP() << "device does not support shared system allocation, "
                      "skipping the test";
    }
  }
}

}; // namespace level_zero_tests
