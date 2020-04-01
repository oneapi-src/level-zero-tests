/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace level_zero_tests {

void *allocate_host_memory(const size_t size) {
  return allocate_host_memory(size, 1);
}

void *allocate_host_memory(const size_t size, const size_t alignment) {
  const ze_host_mem_alloc_flag_t flags = ZE_HOST_MEM_ALLOC_FLAG_DEFAULT;

  ze_driver_handle_t driver = lzt::get_default_driver();
  ze_host_mem_alloc_desc_t host_desc;
  host_desc.flags = flags;
  host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;

  void *memory = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverAllocHostMem(driver, &host_desc, size, alignment, &memory));
  EXPECT_NE(nullptr, memory);

  return memory;
}

void *allocate_device_memory(const size_t size) {
  return (allocate_device_memory(size, 1));
}

void *allocate_device_memory(const size_t size, const size_t alignment) {
  return (allocate_device_memory(size, alignment,
                                 ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT));
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flag_t flags) {

  ze_driver_handle_t driver = lzt::get_default_driver();
  ze_device_handle_t device = zeDevice::get_instance()->get_device();
  return allocate_device_memory(size, alignment, flags, device, driver);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flag_t flags,
                             ze_device_handle_t device,
                             ze_driver_handle_t driver) {

  return allocate_device_memory(size, alignment, flags, 0, device, driver);
}

void *allocate_device_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flag_t flags,
                             const uint32_t ordinal,
                             ze_device_handle_t device_handle,
                             ze_driver_handle_t driver) {
  void *memory = nullptr;
  ze_device_mem_alloc_desc_t device_desc;
  device_desc.ordinal = ordinal;
  device_desc.flags = flags;
  device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverAllocDeviceMem(driver, &device_desc, size, alignment,
                                   device_handle, &memory));
  EXPECT_NE(nullptr, memory);

  return memory;
}
void *allocate_shared_memory(const size_t size) {
  return allocate_shared_memory(size, 1);
}
void *allocate_shared_memory(const size_t size, const size_t alignment) {
  return allocate_shared_memory(size, alignment,
                                ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
                                ZE_HOST_MEM_ALLOC_FLAG_DEFAULT);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flag_t dev_flags,
                             const ze_host_mem_alloc_flag_t host_flags) {

  ze_device_handle_t device = zeDevice::get_instance()->get_device();

  return allocate_shared_memory(size, alignment,
                                ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
                                ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, device);
}

void *allocate_shared_memory(const size_t size, const size_t alignment,
                             const ze_device_mem_alloc_flag_t dev_flags,
                             const ze_host_mem_alloc_flag_t host_flags,
                             ze_device_handle_t device) {

  uint32_t ordinal = 0;
  auto driver = lzt::get_default_driver();

  void *memory = nullptr;
  ze_device_mem_alloc_desc_t device_desc;
  device_desc.ordinal = ordinal;
  device_desc.flags = dev_flags;
  device_desc.version = ZE_DEVICE_MEM_ALLOC_DESC_VERSION_CURRENT;
  ze_host_mem_alloc_desc_t host_desc;
  host_desc.flags = host_flags;
  host_desc.version = ZE_HOST_MEM_ALLOC_DESC_VERSION_CURRENT;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDriverAllocSharedMem(driver, &device_desc, &host_desc, size,
                                   alignment, device, &memory));
  EXPECT_NE(nullptr, memory);

  return memory;
}

void allocate_mem(void **memory, ze_memory_type_t mem_type, size_t size) {
  switch (mem_type) {
  case ZE_MEMORY_TYPE_HOST:
    *memory = allocate_host_memory(size);
    break;
  case ZE_MEMORY_TYPE_DEVICE:
    *memory = allocate_device_memory(size);
    break;
  case ZE_MEMORY_TYPE_SHARED:
    *memory = allocate_shared_memory(size);
    break;
  case ZE_MEMORY_TYPE_UNKNOWN:
  default:
    break;
  }
}

void free_memory(const void *ptr) {
  free_memory(lzt::get_default_driver(), ptr);
}

void free_memory(ze_driver_handle_t driver, const void *ptr) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverFreeMem(driver, (void *)ptr));
}

void allocate_mem_and_get_ipc_handle(ze_ipc_mem_handle_t *mem_handle,
                                     void **memory, ze_memory_type_t mem_type) {
  allocate_mem_and_get_ipc_handle(mem_handle, memory, mem_type, 1);
}

void allocate_mem_and_get_ipc_handle(ze_ipc_mem_handle_t *mem_handle,
                                     void **memory, ze_memory_type_t mem_type,
                                     size_t size) {
  allocate_mem(memory, mem_type, size);
  get_ipc_handle(mem_handle, *memory);
}

void get_ipc_handle(ze_ipc_mem_handle_t *mem_handle, void *memory) {
  EXPECT_EQ(
      ZE_RESULT_SUCCESS,
      zeDriverGetMemIpcHandle(lzt::get_default_driver(), memory, mem_handle));
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
    EXPECT_EQ(pbuff[i], dp);
    dp = (dp + data_pattern) & 0xff;
  }
}
void get_mem_alloc_properties(
    ze_driver_handle_t driver, const void *memory,
    ze_memory_allocation_properties_t *memory_properties) {
  get_mem_alloc_properties(driver, memory, memory_properties, nullptr);
}
void get_mem_alloc_properties(
    ze_driver_handle_t driver, const void *memory,
    ze_memory_allocation_properties_t *memory_properties,
    ze_device_handle_t *device) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetMemAllocProperties(
                                   driver, memory, memory_properties, device));
}
}; // namespace level_zero_tests
