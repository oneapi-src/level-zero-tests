/*
 *
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#ifdef __linux__
#include <unistd.h>
#endif

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

using lzt::to_int;

void cleanup_mapped_virtual_memory(ze_context_handle_t context,
                                   void *&reservedVirtualMemory,
                                   size_t virtualReservationSize,
                                   ze_physical_mem_handle_t *physicalMemories,
                                   size_t physicalMemoryCount, void *&memory,
                                   lzt::command_bundle &bundle) {
  if (reservedVirtualMemory != nullptr) {
    // Releases the reservation and any mappings associated with it.
    lzt::virtual_memory_free(context, reservedVirtualMemory,
                             virtualReservationSize);
    reservedVirtualMemory = nullptr;
  }

  for (size_t i = 0; i < physicalMemoryCount; i++) {
    auto &physicalMemory = physicalMemories[i];
    if (physicalMemory != nullptr) {
      lzt::physical_memory_destroy(context, physicalMemory);
      physicalMemory = nullptr;
    }
  }

  if (memory != nullptr) {
    lzt::free_memory(memory);
    memory = nullptr;
  }

  lzt::destroy_command_bundle(bundle);
}

class zeVirtualMemoryTests : public ::testing::Test {
protected:
  void SetUp() override {
    context = lzt::get_default_context();
    device = lzt::zeDevice::get_instance()->get_device();
  }
  void TearDown() override {}

public:
  bool
  set_virtual_mem_access_and_verify(ze_memory_access_attribute_t new_access) {
    ze_memory_access_attribute_t access =
        ZE_MEMORY_ACCESS_ATTRIBUTE_FORCE_UINT32;
    size_t memory_size = 0;
    ze_result_t result = zeVirtualMemGetAccessAttribute(
        context, reservedVirtualMemory, allocationSize, &access, &memory_size);
    EXPECT_ZE_RESULT_SUCCESS(result);
    LOG_INFO << "Changing virtual memory access from " << lzt::to_string(access)
             << " to " << lzt::to_string(new_access) << '\n';
    result = zeVirtualMemSetAccessAttribute(context, reservedVirtualMemory,
                                            allocationSize, new_access);
    if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      return false;
    }
    EXPECT_ZE_RESULT_SUCCESS(result);
    result = zeVirtualMemGetAccessAttribute(
        context, reservedVirtualMemory, allocationSize, &access, &memory_size);
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_EQ(access, new_access);
    EXPECT_GE(memory_size, allocationSize);
    return true;
  }

  ze_context_handle_t context;
  ze_device_handle_t device;
  size_t pageSize = 1ul << 21;
  size_t allocationSize = (1024 * 1024);
  void *reservedVirtualMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalDeviceMemory = nullptr;
  ze_physical_mem_handle_t reservedPhysicalHostMemory = nullptr;
};

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenNullStartAddressAndValidSizeTheVirtualMemoryReserveReturnsSuccess) {

  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenValidStartAddressAndNewValidSizeThenResizingVirtualReservationSucceeds) {

  size_t largeAllocSize = allocationSize * 4;
  lzt::query_page_size(context, device, largeAllocSize, &pageSize);
  largeAllocSize = lzt::create_page_aligned_size(largeAllocSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, largeAllocSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, largeAllocSize);
  size_t smallerAllocSize = allocationSize * 2;
  lzt::query_page_size(context, device, smallerAllocSize, &pageSize);
  smallerAllocSize = lzt::create_page_aligned_size(smallerAllocSize, pageSize);
  lzt::virtual_memory_reservation(context, reservedVirtualMemory,
                                  smallerAllocSize, &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, smallerAllocSize);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenVirtualReservationWithCustomStartAddressThenResizedPtrAllocated) {

  void *originalPtr = nullptr;
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &originalPtr);
  EXPECT_NE(nullptr, originalPtr);

  void *newUpdatedPtr = reinterpret_cast<void *>(
      reinterpret_cast<size_t>(originalPtr) + allocationSize);
  void *recievedPtr = nullptr;
  lzt::virtual_memory_reservation(context, newUpdatedPtr, allocationSize,
                                  &recievedPtr);
  EXPECT_NE(nullptr, recievedPtr);
  if (recievedPtr != newUpdatedPtr) {
    lzt::virtual_memory_free(context, recievedPtr, allocationSize);
    size_t updatedSize = allocationSize + allocationSize;
    lzt::query_page_size(context, device, updatedSize, &pageSize);
    updatedSize = lzt::create_page_aligned_size(updatedSize, pageSize);
    void *largerPtr = nullptr;
    lzt::virtual_memory_reservation(context, nullptr, updatedSize, &largerPtr);
    EXPECT_NE(nullptr, largerPtr);
    lzt::virtual_memory_free(context, originalPtr, allocationSize);
  }
}

void RunGivenVirtualMemoryReservationThenSettingTheMemoryAccessAttribute(
    zeVirtualMemoryTests &test, bool is_host_memory,
    lzt::command_list_mode_t mode) {
  ze_memory_access_attribute_t access = ZE_MEMORY_ACCESS_ATTRIBUTE_FORCE_UINT32;
  size_t memorySize = 0;
  lzt::query_page_size(test.context, test.device, test.allocationSize,
                       &test.pageSize);
  test.allocationSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  lzt::virtual_memory_reservation(test.context, nullptr, test.allocationSize,
                                  &test.reservedVirtualMemory);
  ze_physical_mem_handle_t reservedPhysicalMemory = nullptr;
  if (is_host_memory) {
    lzt::physical_host_memory_allocation(test.context, test.allocationSize,
                                         &reservedPhysicalMemory);
  } else {
    lzt::physical_device_memory_allocation(test.context, test.device,
                                           test.allocationSize,
                                           &reservedPhysicalMemory);
  }

  EXPECT_NE(nullptr, reservedPhysicalMemory);
  EXPECT_NE(nullptr, test.reservedVirtualMemory);
  lzt::virtual_memory_reservation_get_access(
      test.context, test.reservedVirtualMemory, test.allocationSize, &access,
      &memorySize);
  EXPECT_EQ(access, ZE_MEMORY_ACCESS_ATTRIBUTE_NONE);
  EXPECT_GE(memorySize, test.allocationSize);

  void *memory_in =
      lzt::allocate_shared_memory(test.allocationSize, test.pageSize);
  void *memory_out =
      lzt::allocate_shared_memory(test.allocationSize, test.pageSize);

  const uint32_t input_pattern = 0x99999999;
  const uint32_t output_pattern = 0x66666666;

  auto bundle = lzt::create_command_bundle(test.device, mode);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE, ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY,
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE};

  for (auto accessFlags : memoryAccessFlags) {
    LOG_INFO << "ze_memory_access_attribute_t: " << lzt::to_string(accessFlags);
    bool map_failed = false;
    ze_result_t map_result = ZE_RESULT_SUCCESS;

    switch (accessFlags) {
    case ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE: {
      lzt::virtual_memory_map(test.context, test.reservedVirtualMemory,
                              test.allocationSize, reservedPhysicalMemory, 0,
                              ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, map_result);
      if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_NONE not supported, "
                       "skipping this permutation";
        break;
      }
      if (map_result != ZE_RESULT_SUCCESS) {
        map_failed = true;
        break;
      }
      if (!test.set_virtual_mem_access_and_verify(
              ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE)) {
        LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE not supported, "
                       "skipping this permutation";
        lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                  test.allocationSize);
        break;
      }

      lzt::append_memory_fill(bundle.record_list(), memory_out, &output_pattern,
                              sizeof(output_pattern), test.allocationSize,
                              nullptr);
      lzt::append_memory_fill(bundle.record_list(), memory_in, &input_pattern,
                              sizeof(input_pattern), test.allocationSize,
                              nullptr);
      lzt::append_barrier(bundle.record_list(), nullptr, 0, nullptr);
      lzt::append_memory_copy(bundle.record_list(), test.reservedVirtualMemory,
                              memory_in, test.allocationSize, nullptr);
      lzt::append_barrier(bundle.record_list(), nullptr, 0, nullptr);
      lzt::append_memory_copy(bundle.record_list(), memory_out,
                              test.reservedVirtualMemory, test.allocationSize,
                              nullptr);
      lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
      EXPECT_EQ(reinterpret_cast<uint32_t *>(memory_out)[0], input_pattern);

      lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                test.allocationSize);
      break;
    }
    case ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY: {
      const auto readonly_capability =
          lzt::get_device_readonly_memory_ext_properties(test.device)
              .readonlyCapability;
      LOG_INFO << "ze_device_readonly_memory_capability_t: "
               << lzt::to_string(readonly_capability);
      if (readonly_capability ==
          ZE_DEVICE_READONLY_MEMORY_CAPABILITY_ENFORCED) {
        // Read-only is hardware-enforced: writing the range would fault.
        // The write is never issued against the read-only range.
        lzt::virtual_memory_map(test.context, test.reservedVirtualMemory,
                                test.allocationSize, reservedPhysicalMemory, 0,
                                ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, map_result);
        if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(enforced mode), skipping this permutation";
          break;
        }
        if (map_result != ZE_RESULT_SUCCESS) {
          map_failed = true;
          break;
        }
        if (!test.set_virtual_mem_access_and_verify(
                ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE)) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(enforced mode setup), skipping this permutation";
          lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                    test.allocationSize);
          break;
        }
        lzt::append_memory_fill(
            bundle.record_list(), test.reservedVirtualMemory, &input_pattern,
            sizeof(input_pattern), test.allocationSize, nullptr);
        lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
        lzt::reset_command_bundle(bundle);
        lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                  test.allocationSize);

        lzt::virtual_memory_map(test.context, test.reservedVirtualMemory,
                                test.allocationSize, reservedPhysicalMemory, 0,
                                ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, map_result);
        if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(enforced mode verify), skipping this permutation";
          break;
        }
        if (map_result != ZE_RESULT_SUCCESS) {
          map_failed = true;
          break;
        }
        if (!test.set_virtual_mem_access_and_verify(
                ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY)) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(enforced mode verify), skipping this permutation";
          lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                    test.allocationSize);
          break;
        }

        // Reads are permitted on a read-only range, so the seeded value must
        // read back.
        lzt::append_memory_fill(bundle.record_list(), memory_out,
                                &output_pattern, sizeof(output_pattern),
                                test.allocationSize, nullptr);
        lzt::append_barrier(bundle.record_list(), nullptr, 0, nullptr);
        lzt::append_memory_copy(bundle.record_list(), memory_out,
                                test.reservedVirtualMemory, test.allocationSize,
                                nullptr);
        lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
        EXPECT_EQ(reinterpret_cast<uint32_t *>(memory_out)[0], input_pattern);
      } else {
        // Read-only has no effect (NONE) or is a non-faulting hint: a GPU write
        // to the range will not fault.
        lzt::virtual_memory_map(test.context, test.reservedVirtualMemory,
                                test.allocationSize, reservedPhysicalMemory, 0,
                                ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, map_result);
        if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(non-enforced mode), skipping this permutation";
          break;
        }
        if (map_result != ZE_RESULT_SUCCESS) {
          map_failed = true;
          break;
        }
        if (!test.set_virtual_mem_access_and_verify(
                ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY)) {
          LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported "
                         "(non-enforced mode), skipping this permutation";
          lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                    test.allocationSize);
          break;
        }

        lzt::append_memory_fill(bundle.record_list(), memory_out,
                                &output_pattern, sizeof(output_pattern),
                                test.allocationSize, nullptr);
        lzt::append_memory_fill(bundle.record_list(), memory_in, &input_pattern,
                                sizeof(input_pattern), test.allocationSize,
                                nullptr);
        lzt::append_barrier(bundle.record_list(), nullptr, 0, nullptr);
        lzt::append_memory_copy(bundle.record_list(),
                                test.reservedVirtualMemory, memory_in,
                                test.allocationSize, nullptr);
        lzt::append_barrier(bundle.record_list(), nullptr, 0, nullptr);
        lzt::append_memory_copy(bundle.record_list(), memory_out,
                                test.reservedVirtualMemory, test.allocationSize,
                                nullptr);
        lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
        if (readonly_capability == ZE_DEVICE_READONLY_MEMORY_CAPABILITY_HINT) {
          LOG_WARNING << "Read-only is a performance hint; write result is "
                         "unpredictable, skipping value check.";
        } else {
          EXPECT_EQ(reinterpret_cast<uint32_t *>(memory_out)[0], input_pattern);
        }
      }

      lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                test.allocationSize);
      break;
    }
    case ZE_MEMORY_ACCESS_ATTRIBUTE_NONE:
    default:
      // The range is inaccessible; any GPU read or write would fault on an
      // enforcing device, so only the map/get access round-trip is validated.
      lzt::virtual_memory_map(test.context, test.reservedVirtualMemory,
                              test.allocationSize, reservedPhysicalMemory, 0,
                              ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE, map_result);
      if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_NONE not supported, "
                       "skipping this permutation";
        break;
      }
      if (map_result != ZE_RESULT_SUCCESS) {
        map_failed = true;
        break;
      }
      if (!test.set_virtual_mem_access_and_verify(
              ZE_MEMORY_ACCESS_ATTRIBUTE_NONE)) {
        LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_NONE not supported, "
                       "skipping this permutation";
        lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                  test.allocationSize);
        break;
      }
      lzt::virtual_memory_unmap(test.context, test.reservedVirtualMemory,
                                test.allocationSize);
      break;
    }

    lzt::reset_command_bundle(bundle);

    if (map_failed) {
      ADD_FAILURE() << "zeVirtualMemMap failed with result " << map_result;
      LOG_WARNING << "Stopping test after zeVirtualMemMap failure.";
      break;
    }
  }

  lzt::free_memory(memory_in);
  lzt::free_memory(memory_out);
  lzt::physical_memory_destroy(test.context, reservedPhysicalMemory);
  lzt::virtual_memory_free(test.context, test.reservedVirtualMemory,
                           test.allocationSize);
  lzt::destroy_command_bundle(bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenHostVirtualMemoryReservationThenSettingTheMemoryAccessAttributeReturnsSuccess) {
  RunGivenVirtualMemoryReservationThenSettingTheMemoryAccessAttribute(
      *this, true, lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenHostVirtualMemoryReservationThenSettingTheMemoryAccessAttributeOnImmediateCmdListReturnsSuccess) {
  RunGivenVirtualMemoryReservationThenSettingTheMemoryAccessAttribute(
      *this, true, lzt::command_list_mode_t::immediate);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenDeviceVirtualMemoryReservationThenSettingTheMemoryAccessAttributeReturnsSuccess) {
  RunGivenVirtualMemoryReservationThenSettingTheMemoryAccessAttribute(
      *this, false, lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenDeviceVirtualMemoryReservationThenSettingTheMemoryAccessAttributeOnImmediateCmdListReturnsSuccess) {
  RunGivenVirtualMemoryReservationThenSettingTheMemoryAccessAttribute(
      *this, false, lzt::command_list_mode_t::immediate);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPageAlignedSizeThenVirtualAndPhysicalMemoryReservedSuccessfully) {
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_device_memory_allocation(context, device, allocationSize,
                                         &reservedPhysicalDeviceMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalDeviceMemory);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPageAlignedSizeThenVirtualAndPhysicalHostMemoryReservedSuccessfully) {
#ifdef __linux__
  const long os_page_size = sysconf(_SC_PAGE_SIZE);
  if (os_page_size > 0) {
    pageSize = static_cast<size_t>(os_page_size);
  }
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_host_memory_allocation(context, allocationSize,
                                       &reservedPhysicalHostMemory);
  lzt::physical_memory_destroy(context, reservedPhysicalHostMemory);
#else
  GTEST_SKIP() << "Physical host memory is unsupported on Windows";
#endif
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPageAlignedSizeThenPhysicalMemoryisSuccessfullyReservedForAllAccessTypes) {
  lzt::query_page_size(context, device, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  lzt::physical_device_memory_allocation(context, device, allocationSize,
                                         &reservedPhysicalDeviceMemory);
#ifdef __linux__
  lzt::physical_host_memory_allocation(context, allocationSize,
                                       &reservedPhysicalHostMemory);
#endif
  lzt::virtual_memory_reservation(context, nullptr, allocationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  std::vector<ze_memory_access_attribute_t> memoryAccessFlags = {
      ZE_MEMORY_ACCESS_ATTRIBUTE_NONE, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE,
      ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY};

  for (auto accessFlags : memoryAccessFlags) {
    ze_result_t map_result = ZE_RESULT_SUCCESS;
    lzt::virtual_memory_map(context, reservedVirtualMemory, allocationSize,
                            reservedPhysicalDeviceMemory, 0, accessFlags,
                            map_result);
    if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_WARNING
          << "Access attribute " << lzt::to_string(accessFlags)
          << " not supported on device memory, skipping this permutation";
      continue;
    }
    EXPECT_ZE_RESULT_SUCCESS(map_result);
    lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);
  }
#ifdef __linux__
  for (auto accessFlags : memoryAccessFlags) {
    ze_result_t map_result = ZE_RESULT_SUCCESS;
    lzt::virtual_memory_map(context, reservedVirtualMemory, allocationSize,
                            reservedPhysicalHostMemory, 0, accessFlags,
                            map_result);
    if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_WARNING << "Access attribute " << lzt::to_string(accessFlags)
                  << " not supported on host memory, skipping this permutation";
      continue;
    }
    EXPECT_ZE_RESULT_SUCCESS(map_result);
    lzt::virtual_memory_unmap(context, reservedVirtualMemory, allocationSize);
  }

  lzt::physical_memory_destroy(context, reservedPhysicalHostMemory);
#endif
  lzt::physical_memory_destroy(context, reservedPhysicalDeviceMemory);
  lzt::virtual_memory_free(context, reservedVirtualMemory, allocationSize);
}

void RunGivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemory(
    zeVirtualMemoryTests &test, bool is_host_memory,
    lzt::command_list_mode_t mode) {
  auto bundle = lzt::create_command_bundle(test.device, mode);
  ze_physical_mem_handle_t physicalMemory = nullptr;
  void *memory = nullptr;

  if (is_host_memory) {
#ifdef __linux__
    const long os_page_size = sysconf(_SC_PAGE_SIZE);
    if (os_page_size > 0) {
      test.pageSize = static_cast<size_t>(os_page_size);
    }
#endif
  } else {
    lzt::query_page_size(test.context, test.device, test.allocationSize,
                         &test.pageSize);
  }

  test.allocationSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  lzt::virtual_memory_reservation(test.context, nullptr, test.allocationSize,
                                  &test.reservedVirtualMemory);

  EXPECT_NE(nullptr, test.reservedVirtualMemory);
  if (is_host_memory) {
    lzt::physical_host_memory_allocation(test.context, test.allocationSize,
                                         &physicalMemory);
  } else {
    lzt::physical_device_memory_allocation(
        test.context, test.device, test.allocationSize, &physicalMemory);
  }
  EXPECT_NE(nullptr, physicalMemory);

  const ze_result_t map_result = zeVirtualMemMap(
      test.context, test.reservedVirtualMemory, test.allocationSize,
      physicalMemory, 0, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
  if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "Virtual memory access attributes not supported";
    cleanup_mapped_virtual_memory(test.context, test.reservedVirtualMemory,
                                  test.allocationSize, &physicalMemory, 1,
                                  memory, bundle);
    GTEST_SKIP();
  }
  ASSERT_ZE_RESULT_SUCCESS(map_result);

  int8_t pattern = 9;
  memory = lzt::allocate_shared_memory(test.allocationSize, test.pageSize);
  lzt::append_memory_fill(bundle.list, test.reservedVirtualMemory, &pattern,
                          sizeof(pattern), test.allocationSize, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, memory, test.reservedVirtualMemory,
                          test.allocationSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (size_t i = 0U; i < test.allocationSize; i++) {
    ASSERT_EQ(data[i], pattern);
  }
  cleanup_mapped_virtual_memory(test.context, test.reservedVirtualMemory,
                                test.allocationSize, &physicalMemory, 1, memory,
                                bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  RunGivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemory(
      *this, false, lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemoryOnImmediateCommandListSucceeds) {
  RunGivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemory(
      *this, false, lzt::command_list_mode_t::immediate);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualHostMemorySucceeds) {
#ifdef __linux__
  RunGivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemory(
      *this, true, lzt::command_list_mode_t::regular);
#else
  GTEST_SKIP() << "Physical host memory is unsupported on Windows";
#endif
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualHostMemoryOnImmediateCommandListSucceeds) {
#ifdef __linux__
  RunGivenMappedReadWriteMemoryThenFillAndCopyWithMappedVirtualMemory(
      *this, true, lzt::command_list_mode_t::immediate);
#else
  GTEST_SKIP() << "Physical host memory is unsupported on Windows";
#endif
}

void RunGivenMappedMultiplePhysicalMemoryAcrossAvailableDevicesWhenFillAndCopyWithSingleMappedVirtualMemory(
    zeVirtualMemoryTests &test, lzt::command_list_mode_t mode) {
  uint32_t numDevices = lzt::get_ze_device_count();
  std::vector<ze_device_handle_t> devices;
  std::vector<ze_device_handle_t> potential_devices =
      lzt::get_ze_devices(numDevices);
  std::vector<ze_physical_mem_handle_t> reservedPhysicalMemoryArray;
  devices.push_back(potential_devices[0]);
  for (uint32_t i = 1; i < numDevices; i++) {
    if (lzt::can_access_peer(devices[0], potential_devices[i])) {
      devices.push_back(potential_devices[i]);
    }
  }
  if (devices.size() == 1u) {
    if (numDevices > 1) {
      LOG_INFO << "Devices cannot access each other, using one device.";
    }
    reservedPhysicalMemoryArray.resize(2);
    devices.resize(2);
    devices[0] = test.device;
    devices[1] = test.device;
  } else {
    reservedPhysicalMemoryArray.resize(devices.size());
  }
  auto bundle = lzt::create_command_bundle(test.context, devices[0], mode);

  lzt::query_page_size(test.context, test.device, 0, &test.pageSize);
  test.allocationSize = test.pageSize;
  test.allocationSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  for (size_t i = 0U; i < devices.size(); i++) {
    lzt::physical_device_memory_allocation(test.context, devices[i],
                                           test.allocationSize,
                                           &reservedPhysicalMemoryArray[i]);
  }

  size_t totalAllocationSize = test.allocationSize * devices.size();
  size_t virtualReservationSize = lzt::nextPowerOfTwo(totalAllocationSize);

  lzt::virtual_memory_reservation(test.context, nullptr, virtualReservationSize,
                                  &test.reservedVirtualMemory);
  EXPECT_NE(nullptr, test.reservedVirtualMemory);

  void *memory = nullptr;

  size_t offset = 0;
  for (size_t i = 0U; i < devices.size(); i++) {
    void *reservedVirtualMemoryOffset = reinterpret_cast<void *>(
        reinterpret_cast<uint64_t>(test.reservedVirtualMemory) + offset);
    ze_result_t map_result =
        zeVirtualMemMap(test.context, reservedVirtualMemoryOffset,
                        test.allocationSize, reservedPhysicalMemoryArray[i], 0,
                        ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
    if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_WARNING << "Virtual memory access attributes not supported";
      cleanup_mapped_virtual_memory(
          test.context, test.reservedVirtualMemory, virtualReservationSize,
          reservedPhysicalMemoryArray.data(),
          reservedPhysicalMemoryArray.size(), memory, bundle);
      GTEST_SKIP();
    }
    ASSERT_ZE_RESULT_SUCCESS(map_result);
    offset += test.allocationSize;
  }

  int8_t pattern = 9;
  memory = lzt::allocate_shared_memory(totalAllocationSize, test.pageSize);
  lzt::append_memory_fill(bundle.list, test.reservedVirtualMemory, &pattern,
                          sizeof(pattern), totalAllocationSize, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, memory, test.reservedVirtualMemory,
                          totalAllocationSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (size_t i = 0U; i < totalAllocationSize; i++) {
    ASSERT_EQ(data[i], pattern);
  }
  cleanup_mapped_virtual_memory(
      test.context, test.reservedVirtualMemory, virtualReservationSize,
      reservedPhysicalMemoryArray.data(), reservedPhysicalMemoryArray.size(),
      memory, bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedMultiplePhysicalMemoryAcrossAvailableDevicesWhenFillAndCopyWithSingleMappedVirtualMemoryThenMemoryCheckSucceeds) {
  RunGivenMappedMultiplePhysicalMemoryAcrossAvailableDevicesWhenFillAndCopyWithSingleMappedVirtualMemory(
      *this, lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMappedMultiplePhysicalMemoryAcrossAvailableDevicesWhenFillAndCopyWithSingleMappedVirtualMemoryOnImmediateCmdListThenMemoryCheckSucceeds) {
  RunGivenMappedMultiplePhysicalMemoryAcrossAvailableDevicesWhenFillAndCopyWithSingleMappedVirtualMemory(
      *this, lzt::command_list_mode_t::immediate);
}

void RunGivenVirtualMemoryMappedToMultipleAllocationsWhenFullAddressUsageInKernel(
    zeVirtualMemoryTests &test, lzt::command_list_mode_t mode) {
  uint32_t numDevices = lzt::get_ze_device_count();
  std::vector<ze_device_handle_t> devices;
  std::vector<ze_device_handle_t> potential_devices =
      lzt::get_ze_devices(numDevices);
  std::vector<ze_physical_mem_handle_t> reservedPhysicalMemoryArray;
  devices.push_back(potential_devices[0]);
  for (uint32_t i = 1; i < numDevices; i++) {
    if (lzt::can_access_peer(devices[0], potential_devices[i])) {
      devices.push_back(potential_devices[i]);
    }
  }
  if (devices.size() == 1u) {
    if (numDevices > 1) {
      LOG_INFO << "Devices cannot access each other, using one device.";
    }
    reservedPhysicalMemoryArray.resize(2);
    devices.resize(2);
    devices[0] = test.device;
    devices[1] = test.device;
  } else {
    reservedPhysicalMemoryArray.resize(devices.size());
  }
  auto bundle = lzt::create_command_bundle(test.context, devices[0], mode);

  lzt::query_page_size(test.context, test.device, 0, &test.pageSize);
  test.allocationSize = test.pageSize;
  test.allocationSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  for (size_t i = 0U; i < devices.size(); i++) {
    lzt::physical_device_memory_allocation(test.context, devices[i],
                                           test.allocationSize,
                                           &reservedPhysicalMemoryArray[i]);
  }
  size_t totalAllocationSize = test.allocationSize * devices.size();
  size_t virtualReservationSize = lzt::nextPowerOfTwo(totalAllocationSize);

  lzt::virtual_memory_reservation(test.context, nullptr, virtualReservationSize,
                                  &test.reservedVirtualMemory);
  EXPECT_NE(nullptr, test.reservedVirtualMemory);

  void *memory = nullptr;

  size_t offset = 0;
  for (size_t i = 0U; i < devices.size(); i++) {
    void *reservedVirtualMemoryOffset = reinterpret_cast<void *>(
        reinterpret_cast<uint64_t>(test.reservedVirtualMemory) + offset);
    ze_result_t map_result =
        zeVirtualMemMap(test.context, reservedVirtualMemoryOffset,
                        test.allocationSize, reservedPhysicalMemoryArray[i], 0,
                        ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
    if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_WARNING << "Virtual memory access attributes not supported";
      cleanup_mapped_virtual_memory(
          test.context, test.reservedVirtualMemory, virtualReservationSize,
          reservedPhysicalMemoryArray.data(),
          reservedPhysicalMemoryArray.size(), memory, bundle);
      GTEST_SKIP();
    }
    ASSERT_ZE_RESULT_SUCCESS(map_result);
    offset += test.allocationSize;
  }
  memory = lzt::allocate_shared_memory(totalAllocationSize, test.pageSize);
  lzt::write_data_pattern(memory, totalAllocationSize, 1);
  std::string module_name = "write_memory_pattern.spv";
  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), module_name);
  std::string func_name = "write_memory_pattern";
  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  arg.arg_size = sizeof(uint8_t *);
  arg.arg_value = &test.reservedVirtualMemory;
  args.push_back(arg);
  arg.arg_size = sizeof(int);
  int size = to_int(totalAllocationSize);
  arg.arg_value = &size;
  args.push_back(arg);

  ze_kernel_handle_t function = lzt::create_function(module, func_name);
  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  EXPECT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(
      function, 1, 1, 1, &group_size_x, &group_size_y, &group_size_z));

  EXPECT_ZE_RESULT_SUCCESS(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));

  uint32_t i = 0;
  for (auto arg : args) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(function, i++, arg.arg_size, arg.arg_value));
  }

  ze_group_count_t thread_group_dimensions;
  thread_group_dimensions.groupCountX = 1;
  thread_group_dimensions.groupCountY = 1;
  thread_group_dimensions.groupCountZ = 1;

  uint8_t pattern = 1;
  lzt::append_memory_fill(bundle.list, test.reservedVirtualMemory, &pattern,
                          sizeof(pattern), totalAllocationSize, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);

  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
      bundle.list, function, &thread_group_dimensions, nullptr, 0, nullptr));

  EXPECT_ZE_RESULT_SUCCESS(
      zeCommandListAppendBarrier(bundle.list, nullptr, 0, nullptr));

  lzt::append_memory_copy(bundle.list, memory, test.reservedVirtualMemory,
                          totalAllocationSize, nullptr);

  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

  lzt::validate_data_pattern(memory, totalAllocationSize, -1);

  lzt::destroy_function(function);
  lzt::destroy_module(module);
  cleanup_mapped_virtual_memory(
      test.context, test.reservedVirtualMemory, virtualReservationSize,
      reservedPhysicalMemoryArray.data(), reservedPhysicalMemoryArray.size(),
      memory, bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenVirtualMemoryMappedToMultipleAllocationsWhenFullAddressUsageInKernelThenResultsinValidData) {
  RunGivenVirtualMemoryMappedToMultipleAllocationsWhenFullAddressUsageInKernel(
      *this, lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenVirtualMemoryMappedToMultipleAllocationsWhenFullAddressUsageInKernelOnImmediateCmdListThenResultsinValidData) {
  RunGivenVirtualMemoryMappedToMultipleAllocationsWhenFullAddressUsageInKernel(
      *this, lzt::command_list_mode_t::immediate);
}

enum MemoryReservationTestType {
  MEMORY_RESERVATION_SINGLE_DEVICE,
  MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES,
  MEMORY_RESERVATION_MULTI_ROOT_DEVICES
};

void dataCheckMemoryReservations(enum MemoryReservationTestType type,
                                 lzt::command_list_mode_t mode) {
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t rootDevice = lzt::zeDevice::get_instance()->get_device();
  std::vector<ze_device_handle_t> devices;
  size_t pageSize = 0;
  void *reservedVirtualMemory = nullptr;
  std::vector<ze_physical_mem_handle_t> reservedPhysicalMemory;
  size_t allocationSize = (1024 * 1024);
  uint32_t numDevices, numSubDevices;

  switch (type) {
  case MemoryReservationTestType::MEMORY_RESERVATION_MULTI_ROOT_DEVICES:
    numDevices = lzt::get_ze_device_count();
    if (numDevices < 2) {
      GTEST_SKIP() << "Multi Root Devices not found, skipping test";
    }
    allocationSize = allocationSize * numDevices;
    devices = lzt::get_ze_devices(numDevices);
    reservedPhysicalMemory.resize(numDevices);
    break;
  case MemoryReservationTestType::
      MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES:
    numSubDevices = lzt::get_ze_sub_device_count(rootDevice);
    if (numSubDevices < 2) {
      GTEST_SKIP() << "Multi Sub Devices not found, skipping test";
    }
    allocationSize = allocationSize * numSubDevices;
    devices = lzt::get_ze_sub_devices(rootDevice);
    reservedPhysicalMemory.resize(numSubDevices);
    break;
  case MemoryReservationTestType::MEMORY_RESERVATION_SINGLE_DEVICE:
    reservedPhysicalMemory.resize(2);
    devices.resize(2);
    devices[0] = rootDevice;
    devices[1] = rootDevice;
    break;
  default:
    FAIL() << "Invalid Memory Reservation Test Type";
  }

  auto bundle = lzt::create_command_bundle(rootDevice, mode);

  void *memory = nullptr;

  lzt::query_page_size(context, rootDevice, allocationSize, &pageSize);
  allocationSize = lzt::create_page_aligned_size(allocationSize, pageSize);
  for (size_t i = 0U; i < devices.size(); i++) {
    lzt::physical_device_memory_allocation(context, devices[i], allocationSize,
                                           &reservedPhysicalMemory[i]);
  }
  size_t virtualReservationSize =
      lzt::nextPowerOfTwo(allocationSize * devices.size());
  lzt::virtual_memory_reservation(context, nullptr, virtualReservationSize,
                                  &reservedVirtualMemory);
  EXPECT_NE(nullptr, reservedVirtualMemory);

  size_t offset = 0;
  for (size_t i = 0U; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    ze_result_t map_result = zeVirtualMemMap(
        context, reinterpret_cast<void *>(offsetAddr), allocationSize,
        reservedPhysicalMemory[i], 0, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
    if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      LOG_WARNING << "Virtual memory access attributes not supported";
      cleanup_mapped_virtual_memory(
          context, reservedVirtualMemory, virtualReservationSize,
          reservedPhysicalMemory.data(), reservedPhysicalMemory.size(), memory,
          bundle);
      GTEST_SKIP();
    }
    ASSERT_ZE_RESULT_SUCCESS(map_result);
    offset += allocationSize;
  }

  int8_t pattern = 9;
  memory = lzt::allocate_host_memory(allocationSize * devices.size(), pageSize);

  offset = 0;
  for (size_t i = 0U; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    lzt::append_memory_fill(bundle.list, reinterpret_cast<void *>(offsetAddr),
                            &pattern, sizeof(pattern), allocationSize, nullptr);
    offset += allocationSize;
  }

  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);

  offset = 0;
  for (size_t i = 0U; i < devices.size(); i++) {
    uint64_t offsetAddr =
        reinterpret_cast<uint64_t>(reservedVirtualMemory) + offset;
    uint64_t offsetHostAddr = reinterpret_cast<uint64_t>(memory) + offset;
    lzt::append_memory_copy(
        bundle.list, reinterpret_cast<void *>(offsetHostAddr),
        reinterpret_cast<void *>(offsetAddr), allocationSize, nullptr);
    offset += allocationSize;
  }

  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  uint8_t *data = reinterpret_cast<uint8_t *>(memory);
  for (size_t i = 0U; i < allocationSize * devices.size(); i++) {
    ASSERT_EQ(data[i], pattern);
  }
  cleanup_mapped_virtual_memory(context, reservedVirtualMemory,
                                virtualReservationSize,
                                reservedPhysicalMemory.data(),
                                reservedPhysicalMemory.size(), memory, bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnOneDeviceThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_SINGLE_DEVICE,
      lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnOneDeviceThenFillAndCopyWithMappedVirtualMemoryOnImmediateCmdListSucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_SINGLE_DEVICE,
      lzt::command_list_mode_t::immediate);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnSingleRootDeviceButAcrossSubDevicesThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::
          MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES,
      lzt::command_list_mode_t::regular);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnSingleRootDeviceButAcrossSubDevicesThenFillAndCopyWithMappedVirtualMemoryOnImmediateCmdListSucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::
          MEMORY_RESERVATION_SINGLE_ROOT_DEVICE_MULTI_SUB_DEVICES,
      lzt::command_list_mode_t::immediate);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnMultipleRootDevicesThenFillAndCopyWithMappedVirtualMemorySucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_MULTI_ROOT_DEVICES,
      lzt::command_list_mode_t::regular);
}
LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenMultiMappedReadWriteMemoryOnMultipleRootDevicesThenFillAndCopyWithMappedVirtualMemoryOnImmediateCmdListSucceeds) {
  dataCheckMemoryReservations(
      MemoryReservationTestType::MEMORY_RESERVATION_MULTI_ROOT_DEVICES,
      lzt::command_list_mode_t::immediate);
}

class zeVirtualMemoryMultiMappingTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_memory_type_t, lzt::command_list_mode_t>> {
protected:
  void SetUp() override {
    device = lzt::get_default_device(lzt::get_default_driver());
    context = lzt::get_default_context();
  }
  void TearDown() override {}

public:
  ze_context_handle_t context = nullptr;
  ze_device_handle_t device = nullptr;
};

LZT_TEST_P(
    zeVirtualMemoryMultiMappingTests,
    givenSinglePhysicalHostMemoryMappedToMultipleVirtualMemoryRangeThenReadAndWriteResultsAreCorrect) {
#ifdef __linux__
  const ze_memory_type_t aux_buffer_type = std::get<0>(GetParam());
  const lzt::command_list_mode_t mode = std::get<1>(GetParam());

  constexpr size_t alloc_size = 1ul << 26;

  void *aux_buffer = nullptr;
  switch (aux_buffer_type) {
  case ZE_MEMORY_TYPE_HOST:
    aux_buffer = lzt::allocate_host_memory(alloc_size, sizeof(int64_t));
    break;
  case ZE_MEMORY_TYPE_DEVICE:
    aux_buffer = lzt::allocate_device_memory(alloc_size, sizeof(int64_t));
    break;
  default:
    aux_buffer = lzt::allocate_shared_memory(alloc_size, sizeof(int64_t));
    break;
  }
  EXPECT_NE(nullptr, aux_buffer);

  ze_physical_mem_handle_t physical_host_memory = nullptr;
  lzt::physical_host_memory_allocation(context, alloc_size,
                                       &physical_host_memory);
  EXPECT_NE(nullptr, physical_host_memory);

  void *virtual_memory_0 = nullptr;
  void *virtual_memory_1 = nullptr;
  void *virtual_memory_2 = nullptr;
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_0);
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_1);
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_2);
  EXPECT_NE(nullptr, virtual_memory_0);
  EXPECT_NE(nullptr, virtual_memory_1);
  EXPECT_NE(nullptr, virtual_memory_2);
  EXPECT_NE(virtual_memory_0, virtual_memory_1);
  EXPECT_NE(virtual_memory_0, virtual_memory_2);
  EXPECT_NE(virtual_memory_1, virtual_memory_2);

  ze_result_t map_result = ZE_RESULT_SUCCESS;
  lzt::virtual_memory_map(context, virtual_memory_0, alloc_size,
                          physical_host_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE, map_result);
  if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "Virtual memory access attributes not supported";
    lzt::virtual_memory_free(context, virtual_memory_0, alloc_size);
    lzt::virtual_memory_free(context, virtual_memory_1, alloc_size);
    lzt::virtual_memory_free(context, virtual_memory_2, alloc_size);
    lzt::physical_memory_destroy(context, physical_host_memory);
    lzt::free_memory(context, aux_buffer);
    GTEST_SKIP();
  }
  EXPECT_ZE_RESULT_SUCCESS(map_result);
  lzt::virtual_memory_map(context, virtual_memory_1, alloc_size,
                          physical_host_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE, map_result);
  if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "Virtual memory access attributes not supported";
    lzt::virtual_memory_unmap(context, virtual_memory_0, alloc_size);
    lzt::virtual_memory_free(context, virtual_memory_0, alloc_size);
    lzt::virtual_memory_free(context, virtual_memory_1, alloc_size);
    lzt::virtual_memory_free(context, virtual_memory_2, alloc_size);
    lzt::physical_memory_destroy(context, physical_host_memory);
    lzt::free_memory(context, aux_buffer);
    GTEST_SKIP();
  }
  EXPECT_ZE_RESULT_SUCCESS(map_result);

  std::fill_n(static_cast<uint8_t *>(virtual_memory_0), alloc_size, 0);
  std::fill_n(static_cast<uint8_t *>(virtual_memory_1), alloc_size, 0);

  // Simple read-write test with cross check
  static_cast<int64_t *>(virtual_memory_0)[(alloc_size / sizeof(int64_t)) / 2] =
      0xdeadbeef;
  EXPECT_EQ(0xdeadbeef,
            static_cast<int64_t *>(
                virtual_memory_1)[(alloc_size / sizeof(int64_t)) / 2]);

  static_cast<int64_t *>(virtual_memory_1)[(alloc_size / sizeof(int64_t)) / 3] =
      0xcafecafe;
  EXPECT_EQ(0xcafecafe,
            static_cast<int64_t *>(
                virtual_memory_0)[(alloc_size / sizeof(int64_t)) / 3]);

  // GPU copy test with cross check
  int8_t seven = 7;
  auto bundle = lzt::create_command_bundle(device, mode);
  lzt::append_memory_fill(bundle.list, aux_buffer, &seven, sizeof(seven),
                          alloc_size, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, virtual_memory_0, aux_buffer, alloc_size,
                          nullptr, 0, nullptr);
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(bundle.list));
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::destroy_command_bundle(bundle);

  for (size_t i = 0; i < alloc_size; i++) {
    if (static_cast<int8_t *>(virtual_memory_1)[i] != seven) {
      FAIL() << "Verification failed";
      break;
    }
  }

  lzt::virtual_memory_unmap(context, virtual_memory_0, alloc_size);
  lzt::virtual_memory_free(context, virtual_memory_0, alloc_size);

  lzt::virtual_memory_unmap(context, virtual_memory_1, alloc_size);
  lzt::virtual_memory_free(context, virtual_memory_1, alloc_size);

  // Make sure data in physical host memory is persistent
  lzt::virtual_memory_map(context, virtual_memory_2, alloc_size,
                          physical_host_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY, map_result);
  if (map_result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    LOG_WARNING << "ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY not supported, "
                   "skipping persistence verification";
  } else {
    EXPECT_ZE_RESULT_SUCCESS(map_result);
    for (size_t i = 0; i < alloc_size; i++) {
      if (static_cast<int8_t *>(virtual_memory_2)[i] != seven) {
        FAIL() << "Verification failed";
        break;
      }
    }
    lzt::virtual_memory_unmap(context, virtual_memory_2, alloc_size);
  }
  lzt::virtual_memory_free(context, virtual_memory_2, alloc_size);

  lzt::physical_memory_destroy(context, physical_host_memory);

  lzt::free_memory(context, aux_buffer);
#else
  GTEST_SKIP() << "Physical host memory is unsupported on Windows";
#endif
}

LZT_TEST_P(
    zeVirtualMemoryMultiMappingTests,
    givenSinglePhysicalDeviceMemoryMappedToMultipleVirtualMemoryRangeThenReadAndWriteResultsAreCorrect) {
  const ze_memory_type_t aux_buffer_type = std::get<0>(GetParam());
  const lzt::command_list_mode_t mode = std::get<1>(GetParam());

  size_t page_size = 0;
  size_t alloc_size = 1ul << 26;
  lzt::query_page_size(context, device, alloc_size, &page_size);
  alloc_size = lzt::create_page_aligned_size(alloc_size, page_size);

  // Auxiliary buffer is used as the source for the GPU fills/copies. Device
  // physical memory is not CPU-accessible, so all reads and writes to the
  // mapped virtual ranges must go through the GPU.
  void *aux_buffer = nullptr;
  switch (aux_buffer_type) {
  case ZE_MEMORY_TYPE_HOST:
    aux_buffer = lzt::allocate_host_memory(alloc_size, sizeof(int64_t));
    break;
  case ZE_MEMORY_TYPE_DEVICE:
    aux_buffer = lzt::allocate_device_memory(alloc_size, sizeof(int64_t));
    break;
  default:
    aux_buffer = lzt::allocate_shared_memory(alloc_size, sizeof(int64_t));
    break;
  }
  EXPECT_NE(nullptr, aux_buffer);

  // CPU-accessible buffers to read back and verify what the GPU observed
  // through each virtual range.
  void *verify_buffer_0 =
      lzt::allocate_shared_memory(alloc_size, sizeof(int64_t));
  void *verify_buffer_1 =
      lzt::allocate_shared_memory(alloc_size, sizeof(int64_t));
  EXPECT_NE(nullptr, verify_buffer_0);
  EXPECT_NE(nullptr, verify_buffer_1);

  ze_physical_mem_handle_t physical_device_memory = nullptr;
  lzt::physical_device_memory_allocation(context, device, alloc_size,
                                         &physical_device_memory);
  EXPECT_NE(nullptr, physical_device_memory);

  void *virtual_memory_0 = nullptr;
  void *virtual_memory_1 = nullptr;
  void *virtual_memory_2 = nullptr;
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_0);
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_1);
  lzt::virtual_memory_reservation(context, nullptr, alloc_size,
                                  &virtual_memory_2);
  EXPECT_NE(nullptr, virtual_memory_0);
  EXPECT_NE(nullptr, virtual_memory_1);
  EXPECT_NE(nullptr, virtual_memory_2);
  EXPECT_NE(virtual_memory_0, virtual_memory_1);
  EXPECT_NE(virtual_memory_0, virtual_memory_2);
  EXPECT_NE(virtual_memory_1, virtual_memory_2);

  // Map the same physical device memory into two distinct virtual ranges.
  lzt::virtual_memory_map(context, virtual_memory_0, alloc_size,
                          physical_device_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);
  lzt::virtual_memory_map(context, virtual_memory_1, alloc_size,
                          physical_device_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE);

  const int8_t pattern_0 = 7;
  const int8_t pattern_1 = 0x5a;

  auto bundle = lzt::create_command_bundle(device, mode);
  // Write pattern_0 through virtual_memory_0, then read it back through
  // virtual_memory_1 to confirm both ranges alias the same physical memory.
  lzt::append_memory_fill(bundle.list, aux_buffer, &pattern_0,
                          sizeof(pattern_0), alloc_size, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, virtual_memory_0, aux_buffer, alloc_size,
                          nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, verify_buffer_0, virtual_memory_1,
                          alloc_size, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  // Now write pattern_1 through virtual_memory_1 and read it back through
  // virtual_memory_0 to confirm aliasing in the other direction.
  lzt::append_memory_fill(bundle.list, aux_buffer, &pattern_1,
                          sizeof(pattern_1), alloc_size, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, virtual_memory_1, aux_buffer, alloc_size,
                          nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_memory_copy(bundle.list, verify_buffer_1, virtual_memory_0,
                          alloc_size, nullptr);
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(bundle.list));
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::destroy_command_bundle(bundle);

  for (size_t i = 0; i < alloc_size; i++) {
    if (static_cast<int8_t *>(verify_buffer_0)[i] != pattern_0) {
      FAIL() << "Verification failed reading virtual_memory_1 at index " << i;
      break;
    }
  }
  for (size_t i = 0; i < alloc_size; i++) {
    if (static_cast<int8_t *>(verify_buffer_1)[i] != pattern_1) {
      FAIL() << "Verification failed reading virtual_memory_0 at index " << i;
      break;
    }
  }

  lzt::virtual_memory_unmap(context, virtual_memory_0, alloc_size);
  lzt::virtual_memory_free(context, virtual_memory_0, alloc_size);

  lzt::virtual_memory_unmap(context, virtual_memory_1, alloc_size);
  lzt::virtual_memory_free(context, virtual_memory_1, alloc_size);

  // Make sure the data in the physical device memory is persistent after
  // unmapping the ranges it was written through.
  lzt::virtual_memory_map(context, virtual_memory_2, alloc_size,
                          physical_device_memory, 0,
                          ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY);
  auto verify_bundle = lzt::create_command_bundle(device, mode);
  lzt::append_memory_copy(verify_bundle.list, verify_buffer_0, virtual_memory_2,
                          alloc_size, nullptr);
  ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(verify_bundle.list));
  lzt::execute_and_sync_command_bundle(verify_bundle, UINT64_MAX);
  lzt::destroy_command_bundle(verify_bundle);
  for (size_t i = 0; i < alloc_size; i++) {
    if (static_cast<int8_t *>(verify_buffer_0)[i] != pattern_1) {
      FAIL() << "Verification failed reading persistent physical memory at "
                "index "
             << i;
      break;
    }
  }
  lzt::virtual_memory_unmap(context, virtual_memory_2, alloc_size);
  lzt::virtual_memory_free(context, virtual_memory_2, alloc_size);

  lzt::physical_memory_destroy(context, physical_device_memory);

  lzt::free_memory(verify_buffer_0);
  lzt::free_memory(verify_buffer_1);
  lzt::free_memory(aux_buffer);
}

INSTANTIATE_TEST_SUITE_P(
    VirtualDeviceMemoryMultiMappingParamsRegular,
    zeVirtualMemoryMultiMappingTests,
    ::testing::Combine(::testing::Values(ZE_MEMORY_TYPE_HOST,
                                         ZE_MEMORY_TYPE_DEVICE,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Values(lzt::command_list_mode_t::regular)));

INSTANTIATE_TEST_SUITE_P(
    VirtualDeviceMemoryMultiMappingParamsImmediate,
    zeVirtualMemoryMultiMappingTests,
    ::testing::Combine(::testing::Values(ZE_MEMORY_TYPE_HOST,
                                         ZE_MEMORY_TYPE_DEVICE,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Values(lzt::command_list_mode_t::immediate)));

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenLargeReservationSizesThenVirtualMemoryReservationSucceedsOrFailsGracefully) {
  const size_t gb = 1ull << 30;
  const size_t tb = 1ull << 40;
  const size_t pb = 1ull << 50;

  // Covers 48-bit VA range (up to 128 TB) and 57-bit VA range (up to 64 PB)
  const std::vector<size_t> test_sizes = {
      1 * gb,  1 * tb,  2 * tb,   4 * tb,   8 * tb,   16 * tb,
      32 * tb, 64 * tb, 128 * tb, 256 * tb, 512 * tb, 1 * pb,
      2 * pb,  4 * pb,  8 * pb,   16 * pb,  32 * pb,  64 * pb,
  };

  void *start_address = reinterpret_cast<void *>(0x7fff0000ull);
  for (auto test_size : test_sizes) {
    size_t aligned_size = test_size;
    lzt::query_page_size(context, device, aligned_size, &pageSize);
    aligned_size = lzt::create_page_aligned_size(aligned_size, pageSize);

    void *ptr = nullptr;
    ze_result_t result =
        zeVirtualMemReserve(context, start_address, aligned_size, &ptr);

    LOG_INFO << "Size: " << (test_size / gb) << " GB"
             << " - Page size: " << pageSize
             << " - Result (zeVirtualMemReserve): " << result
             << " - Ptr: " << ptr;

    if (test_size < 128 * tb) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, result)
          << "Expected success for size " << (test_size / gb) << " GB"
          << " - Result: " << result;
    }

    if (result == ZE_RESULT_SUCCESS) {
      EXPECT_NE(nullptr, ptr);
      lzt::virtual_memory_free(context, ptr, aligned_size);
    } else {
      EXPECT_TRUE(result == ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY ||
                  result == ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY)
          << "Unexpected error for size " << (test_size / gb) << " GB"
          << " - Result: " << result;
    }
  }
}

void RunGivenPhysicalMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
    zeVirtualMemoryTests &test, bool is_immediate) {
  // Query page granularity using the actual allocation size.
  lzt::query_page_size(test.context, test.device, test.allocationSize,
                       &test.pageSize);
  const size_t chunkSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  // Physical memory is two chunks; virtual window is one chunk.
  // Using a virtual window of chunkSize (not 2*chunkSize) avoids any
  // larger-size alignment requirement from the driver.
  const size_t physicalSize = chunkSize * 2;

  ze_physical_mem_handle_t physicalMemory = nullptr;
  lzt::physical_device_memory_allocation(test.context, test.device,
                                         physicalSize, &physicalMemory);
  ASSERT_NE(nullptr, physicalMemory);

  void *virtualMemory = nullptr;
  lzt::virtual_memory_reservation(test.context, nullptr, chunkSize,
                                  &virtualMemory);
  ASSERT_NE(nullptr, virtualMemory);

  auto bundle = lzt::create_command_bundle(test.device, is_immediate);

  // Step 1: map virtual -> physical[0..chunkSize) and fill with patternA
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  const uint8_t patternA = 0xAA;
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_fill(bundle.list, virtualMemory, &patternA,
                          sizeof(patternA), chunkSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  // Step 2: map virtual -> physical[chunkSize..2*chunkSize) (non-zero offset)
  // and overwrite with patternB — this must NOT affect physical[0..chunkSize)
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory,
                      chunkSize, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  const uint8_t patternB = 0xBB;
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_fill(bundle.list, virtualMemory, &patternB,
                          sizeof(patternB), chunkSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  void *readbackBuffer = lzt::allocate_shared_memory(chunkSize, test.pageSize);

  // Step 3: re-map at offset 0 — first chunk must still be patternA
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  const uint8_t *data = reinterpret_cast<const uint8_t *>(readbackBuffer);
  LOG_INFO << "[Step3] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternA=" << static_cast<int>(patternA) << ")";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternA)
        << "Pre-offset region modified unexpectedly at byte " << i;
  }

  // Step 4: re-map at offset chunkSize — second chunk must be patternB
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory,
                      chunkSize, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  LOG_INFO << "[Step4] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternB=" << static_cast<int>(patternB) << ")";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternB)
        << "Offset region not written correctly at byte " << i;
  }

  // Step 5: re-map at offset 0 again — confirm first chunk is still patternA
  // (i.e. writing patternB to the second chunk did not corrupt the first chunk)
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  LOG_INFO << "[Step5] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternA=" << static_cast<int>(patternA)
           << ", must be unchanged after patternB write)";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternA)
        << "First chunk corrupted after patternB write at byte " << i;
  }

  lzt::virtual_memory_free(test.context, virtualMemory, chunkSize);
  lzt::free_memory(readbackBuffer);
  lzt::physical_memory_destroy(test.context, physicalMemory);
  lzt::destroy_command_bundle(bundle);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPhysicalMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetRegionUnchanged) {
  RunGivenPhysicalMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
      *this, false);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPhysicalMemoryMappedAtOffsetOnImmediateCmdListThenDataWrittenToOffsetAndPreOffsetRegionUnchanged) {
  RunGivenPhysicalMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
      *this, true);
}

void RunGivenPhysicalHostMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
    zeVirtualMemoryTests &test, bool is_immediate) {
#ifdef __linux__
  // Query page granularity using the actual allocation size.
  lzt::query_page_size(test.context, test.device, test.allocationSize,
                       &test.pageSize);
  const size_t chunkSize =
      lzt::create_page_aligned_size(test.allocationSize, test.pageSize);
  // Physical host memory is two chunks; virtual window is one chunk.
  const size_t physicalSize = chunkSize * 2;

  ze_physical_mem_handle_t physicalMemory = nullptr;
  lzt::physical_host_memory_allocation(test.context, physicalSize,
                                       &physicalMemory);
  ASSERT_NE(nullptr, physicalMemory);

  void *virtualMemory = nullptr;
  lzt::virtual_memory_reservation(test.context, nullptr, chunkSize,
                                  &virtualMemory);
  ASSERT_NE(nullptr, virtualMemory);

  auto bundle = lzt::create_command_bundle(test.device, is_immediate);

  // Step 1: map virtual -> physical[0..chunkSize) and fill with patternA via
  // device
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  const uint8_t patternA = 0xAA;
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_fill(bundle.list, virtualMemory, &patternA,
                          sizeof(patternA), chunkSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  // Step 2: map virtual -> physical[chunkSize..2*chunkSize) (non-zero offset)
  // and fill with patternB — this must NOT affect physical[0..chunkSize)
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory,
                      chunkSize, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  const uint8_t patternB = 0xBB;
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_fill(bundle.list, virtualMemory, &patternB,
                          sizeof(patternB), chunkSize, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  void *readbackBuffer = lzt::allocate_shared_memory(chunkSize, test.pageSize);

  // Step 3: re-map at offset 0 — first chunk must still be patternA
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  const uint8_t *data = reinterpret_cast<const uint8_t *>(readbackBuffer);
  LOG_INFO << "[Step3] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternA=" << static_cast<int>(patternA) << ")";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternA)
        << "Pre-offset region modified unexpectedly at byte " << i;
  }

  // Step 4: re-map at offset chunkSize — second chunk must be patternB
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory,
                      chunkSize, ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  LOG_INFO << "[Step4] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternB=" << static_cast<int>(patternB) << ")";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternB)
        << "Offset region not written correctly at byte " << i;
  }

  // Step 5: re-map at offset 0 again — confirm first chunk is still patternA
  // (i.e. writing patternB to the second chunk did not corrupt the first chunk)
  ASSERT_ZE_RESULT_SUCCESS(
      zeVirtualMemMap(test.context, virtualMemory, chunkSize, physicalMemory, 0,
                      ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
  lzt::reset_command_list(bundle.list);
  lzt::append_memory_copy(bundle.list, readbackBuffer, virtualMemory, chunkSize,
                          nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);
  lzt::virtual_memory_unmap(test.context, virtualMemory, chunkSize);

  LOG_INFO << "[Step5] data[0..3] = " << static_cast<int>(data[0]) << " "
           << static_cast<int>(data[1]) << " " << static_cast<int>(data[2])
           << " " << static_cast<int>(data[3])
           << " (expected patternA=" << static_cast<int>(patternA)
           << ", must be unchanged after patternB write)";
  for (size_t i = 0; i < chunkSize; i++) {
    ASSERT_EQ(data[i], patternA)
        << "First chunk corrupted after patternB write at byte " << i;
  }

  lzt::virtual_memory_free(test.context, virtualMemory, chunkSize);
  lzt::free_memory(readbackBuffer);
  lzt::physical_memory_destroy(test.context, physicalMemory);
  lzt::destroy_command_bundle(bundle);
#else
  GTEST_SKIP() << "Physical host memory is unsupported on Windows";
#endif
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPhysicalHostMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetRegionUnchanged) {
  RunGivenPhysicalHostMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
      *this, false);
}

LZT_TEST_F(
    zeVirtualMemoryTests,
    GivenPhysicalHostMemoryMappedAtOffsetOnImmediateCmdListThenDataWrittenToOffsetAndPreOffsetRegionUnchanged) {
  RunGivenPhysicalHostMemoryMappedAtOffsetThenDataWrittenToOffsetAndPreOffsetUnchanged(
      *this, true);
}

} // namespace
