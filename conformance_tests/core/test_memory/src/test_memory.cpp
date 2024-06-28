/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeDriverAllocDeviceMemTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_device_mem_alloc_flag_t, size_t, size_t>> {
protected:
  void SetUp() override {
    const ze_device_mem_alloc_flag_t flags = std::get<0>(GetParam());
    size_ = std::get<1>(GetParam());
    alignment_ = std::get<2>(GetParam());
    memory_ = lzt::allocate_device_memory(size_, alignment_, flags);
  }
  void TearDown() override { lzt::free_memory(memory_); }
  size_t size_;
  size_t alignment_;
  void *memory_ = nullptr;
};

class zeDriverAllocDeviceMemParamsTests : public zeDriverAllocDeviceMemTests {};

TEST_P(
    zeDriverAllocDeviceMemParamsTests,
    GivenAllocationFlagsAndSizeAndAlignmentWhenAllocatingDeviceMemoryThenNotNullPointerIsReturned) {
}

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocDeviceMemTestVaryFlagsAndSizeAndAlignment,
    zeDriverAllocDeviceMemParamsTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED,
                          ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED),
        lzt::memory_allocation_sizes, lzt::memory_allocation_alignments));

class zeDriverAllocDeviceMemAlignmentTests
    : public zeDriverAllocDeviceMemTests {};

TEST_P(zeDriverAllocDeviceMemAlignmentTests,
       GivenSizeAndAlignmentWhenAllocatingDeviceMemoryThenPointerIsAligned) {
  EXPECT_NE(nullptr, memory_);
  if (alignment_ != 0) {
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(memory_) % alignment_);
  }
}

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocDeviceMemTestVarySizeAndAlignment,
    zeDriverAllocDeviceMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_small));

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocDeviceMemTestVarySizeAndLargeAlignment,
    zeDriverAllocDeviceMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_large));

class zeMemGetAllocPropertiesTests : public zeDriverAllocDeviceMemTests {};

TEST_P(
    zeMemGetAllocPropertiesTests,
    GivenValidDeviceMemoryPointerWhenGettingPropertiesThenVersionAndTypeReturned) {
  ze_memory_allocation_properties_t memory_properties = {};
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_device_handle_t device_test = device;

  memory_properties.pNext = nullptr;
  memory_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  lzt::get_mem_alloc_properties(context, memory_, &memory_properties,
                                &device_test);
  EXPECT_EQ(ZE_MEMORY_TYPE_DEVICE, memory_properties.type);
  EXPECT_EQ(device, device_test);

  if (size_ > 0) {
    uint8_t *char_mem = static_cast<uint8_t *>(memory_);

    memory_properties.pNext = nullptr;
    memory_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
    device_test = device;
    lzt::get_mem_alloc_properties(context,
                                  static_cast<void *>(char_mem + size_ - 1),
                                  &memory_properties, &device_test);
    EXPECT_EQ(ZE_MEMORY_TYPE_DEVICE, memory_properties.type);
    EXPECT_EQ(device, device_test);
  }
}
TEST_P(
    zeMemGetAllocPropertiesTests,
    GivenPointerToDeviceHandleIsSetToNullWhenGettingMemoryPropertiesThenSuccessIsReturned) {

  auto context = lzt::get_default_context();
  ze_memory_allocation_properties_t memory_properties = {};

  memory_properties.pNext = nullptr;
  memory_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  lzt::get_mem_alloc_properties(context, memory_, &memory_properties);
}

INSTANTIATE_TEST_SUITE_P(
    zeMemGetAllocPropertiesTestVaryFlagsAndSizeAndAlignment,
    zeMemGetAllocPropertiesTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments));

class zeDriverMemGetAddressRangeTests : public zeDriverAllocDeviceMemTests {};

TEST_P(
    zeDriverMemGetAddressRangeTests,
    GivenValidDeviceMemoryPointerWhenGettingAddressRangeThenBaseAddressAndSizeReturned) {

  void *pBase = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory_, &pBase, NULL));
  EXPECT_EQ(pBase, memory_);
  size_t addr_range_size = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(lzt::get_default_context(), memory_, NULL,
                                 &addr_range_size));

  // Get device mem size rounds size up to nearest page size
  EXPECT_GE(addr_range_size, size_);
  pBase = nullptr;
  addr_range_size = 0;
  if (size_ > 0) {
    uint8_t *char_mem = static_cast<uint8_t *>(memory_);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(),
                                   static_cast<void *>(char_mem + size_ - 1),
                                   &pBase, &addr_range_size));
  } else {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(), memory_, &pBase,
                                   &addr_range_size));
  }
  EXPECT_EQ(pBase, memory_);
  // Get device mem size rounds size up to nearest page size
  EXPECT_GE(addr_range_size, size_);
}

INSTANTIATE_TEST_SUITE_P(
    zeDriverMemGetAddressRangeTestVaryFlagsAndSizeAndAlignment,
    zeDriverMemGetAddressRangeTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments));

class zeDriverMemFreeTests : public ::testing::Test {};

TEST_F(
    zeDriverMemFreeTests,
    GivenValidDeviceMemAllocationWhenFreeingDeviceMemoryThenSuccessIsReturned) {
  void *memory = lzt::allocate_device_memory(1);
  lzt::free_memory(memory);
}

TEST_F(
    zeDriverMemFreeTests,
    GivenValidSharedMemAllocationWhenFreeingSharedMemoryThenSuccessIsReturned) {
  void *memory = lzt::allocate_shared_memory(1);
  lzt::free_memory(memory);
}

TEST_F(zeDriverMemFreeTests,
       GivenValidHostMemAllocationWhenFreeingHostMemoryThenSuccessIsReturned) {
  void *memory = lzt::allocate_host_memory(1);
  lzt::free_memory(memory);
}

class zeDriverAllocSharedMemTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_device_mem_alloc_flag_t, ze_host_mem_alloc_flag_t,
                     size_t, size_t>> {

public:
  void RunGivenAllocationFlagsSizeAndAlignmentWhenAllocatingSharedMemoryTest(
      bool is_immediate) {
    const ze_device_mem_alloc_flag_t dev_flags = std::get<0>(GetParam());
    const ze_host_mem_alloc_flag_t host_flags = std::get<1>(GetParam());
    const size_t size = std::get<2>(GetParam());
    const size_t alignment = std::get<3>(GetParam());

    void *memory = nullptr;
    auto driver = lzt::get_default_driver();
    auto device = lzt::get_default_device(driver);
    auto context = lzt::create_context();

    const uint8_t pattern = 0x55;
    // Access from host first
    memory = lzt::allocate_shared_memory(size, alignment, dev_flags, host_flags,
                                         device, context);
    EXPECT_NE(nullptr, memory);
    memset(memory, pattern, size);
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern);
    }
    lzt::free_memory(context, memory);

    // Access from device first
    memory = lzt::allocate_shared_memory(size, alignment, dev_flags, host_flags,
                                         device, context);
    EXPECT_NE(nullptr, memory);
    const uint8_t pattern2 = 0x55;

    auto bundle = lzt::create_command_bundle(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

    lzt::append_memory_fill(bundle.list, memory, &pattern2, 1, size, nullptr);

    lzt::close_command_list(bundle.list);
    lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern2);
    }

    lzt::destroy_command_bundle(bundle);
    lzt::free_memory(context, memory);
    lzt::destroy_context(context);
  }
};

TEST_P(
    zeDriverAllocSharedMemTests,
    GivenAllocationFlagsSizeAndAlignmentWhenAllocatingSharedMemoryThenNotNullPointerIsReturned) {
  RunGivenAllocationFlagsSizeAndAlignmentWhenAllocatingSharedMemoryTest(false);
}

TEST_P(
    zeDriverAllocSharedMemTests,
    GivenAllocationFlagsSizeAndAlignmentWhenAllocatingSharedMemoryOnImmediateCmdListThenNotNullPointerIsReturned) {
  RunGivenAllocationFlagsSizeAndAlignmentWhenAllocatingSharedMemoryTest(true);
}

INSTANTIATE_TEST_SUITE_P(
    TestSharedMemFlagPermutations, zeDriverAllocSharedMemTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_CACHED,
                          ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                          ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT),
        ::testing::Values(0, ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED,
                          ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                          ZE_HOST_MEM_ALLOC_FLAG_BIAS_WRITE_COMBINED,
                          ZE_HOST_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT),
        lzt::memory_allocation_sizes, lzt::memory_allocation_alignments));

class zeDriverAllocSharedMemAlignmentTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_device_mem_alloc_flag_t, ze_host_mem_alloc_flag_t,
                     size_t, size_t>> {
protected:
  void SetUp() override {
    const ze_device_mem_alloc_flag_t dev_flags = std::get<0>(GetParam());
    const ze_host_mem_alloc_flag_t host_flags = std::get<1>(GetParam());
    size_ = std::get<2>(GetParam());
    alignment_ = std::get<3>(GetParam());

    auto driver = lzt::get_default_driver();
    auto device = lzt::get_default_device(driver);
    context_ = lzt::create_context();
    memory_ = lzt::allocate_shared_memory(size_, alignment_, dev_flags,
                                          host_flags, device, context_);
  }

  void TearDown() override {
    lzt::free_memory(context_, memory_);
    lzt::destroy_context(context_);
  }

  size_t size_;
  size_t alignment_;
  void *memory_ = nullptr;
  ze_context_handle_t context_;
};

TEST_P(zeDriverAllocSharedMemAlignmentTests,
       GivenSizeAndAlignmentWhenAllocatingSharedMemoryThenPointerIsAligned) {
  EXPECT_NE(nullptr, memory_);
  if (alignment_ != 0) {
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(memory_) % alignment_);
  }
}

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocSharedMemTestVarySizeAndAlignment,
    zeDriverAllocSharedMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), ::testing::Values(0),
                       lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_small));

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocSharedMemTestVarySizeAndLargeAlignment,
    zeDriverAllocSharedMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), ::testing::Values(0),
                       lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_large));

class zeSharedMemGetPropertiesTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(zeSharedMemGetPropertiesTests,
       GivenSharedAllocationWhenGettingMemPropertiesThenSuccessIsReturned) {
  const size_t size = std::get<0>(GetParam());
  const size_t alignment = std::get<1>(GetParam());
  ze_context_handle_t context = lzt::get_default_context();
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  ze_device_handle_t device_test = device;
  ze_memory_allocation_properties_t mem_properties = {};
  mem_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  void *memory = lzt::allocate_shared_memory(size, alignment, 0, 0, device);
  lzt::get_mem_alloc_properties(context, memory, &mem_properties, &device_test);
  EXPECT_EQ(device_test, device);
  lzt::free_memory(memory);
}

INSTANTIATE_TEST_SUITE_P(TestSharedMemGetPropertiesPermutations,
                         zeSharedMemGetPropertiesTests,
                         ::testing::Combine(lzt::memory_allocation_sizes,
                                            lzt::memory_allocation_alignments));

class zeSharedMemGetAddressRangeTests : public ::testing::Test {};

TEST_F(zeSharedMemGetAddressRangeTests,
       GivenSharedAllocationWhenGettingAddressRangeThenCorrectSizeIsReturned) {
  const size_t size = 1;
  const size_t alignment = 1;

  void *memory = lzt::allocate_shared_memory(size, alignment);
  size_t size_out;

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory, NULL, &size_out));
  EXPECT_GE(size_out, size);
  lzt::free_memory(memory);
}

TEST_F(zeSharedMemGetAddressRangeTests,
       GivenSharedAllocationWhenGettingAddressRangeThenCorrectBaseIsReturned) {
  const size_t size = 1;
  const size_t alignment = 1;

  void *memory = lzt::allocate_shared_memory(size, alignment);
  void *base = nullptr;

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory, &base, NULL));
  EXPECT_EQ(base, memory);
  lzt::free_memory(memory);
}

class zeSharedMemGetAddressRangeParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(
    zeSharedMemGetAddressRangeParameterizedTests,
    GivenSharedAllocationWhenGettingAddressRangeThenCorrectSizeAndBaseIsReturned) {
  const size_t size = std::get<0>(GetParam());
  const size_t alignment = std::get<1>(GetParam());

  void *memory = lzt::allocate_shared_memory(size, alignment);
  void *base = nullptr;
  size_t size_out;

  // Test getting address info from begining of memory range
  uint8_t *mem_target = static_cast<uint8_t *>(memory);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory, &base, &size_out));
  EXPECT_GE(size_out, size);
  EXPECT_EQ(base, memory);

  if (size > 1) {
    // Test getting address info from middle of memory range
    mem_target = static_cast<uint8_t *>(memory) + (size - 1) / 2;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(), mem_target,
                                   &base, &size_out));
    EXPECT_GE(size_out, size);
    EXPECT_EQ(memory, base);

    // Test getting address info from end of memory range
    mem_target = static_cast<uint8_t *>(memory) + (size - 1);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(), mem_target,
                                   &base, &size_out));
    EXPECT_GE(size_out, size);
    EXPECT_EQ(memory, base);
  }
  lzt::free_memory(memory);
}

INSTANTIATE_TEST_SUITE_P(TestSharedMemGetAddressRangePermutations,
                         zeSharedMemGetAddressRangeParameterizedTests,
                         ::testing::Combine(lzt::memory_allocation_sizes,
                                            lzt::memory_allocation_alignments));
class zeDriverAllocHostMemTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_host_mem_alloc_flag_t, size_t, size_t>> {};
TEST_P(
    zeDriverAllocHostMemTests,
    GivenFlagsSizeAndAlignmentWhenAllocatingHostMemoryThenNotNullPointerIsReturned) {

  const ze_host_mem_alloc_flag_t flags = std::get<0>(GetParam());

  const size_t size = std::get<1>(GetParam());
  const size_t alignment = std::get<2>(GetParam());

  void *memory = nullptr;
  ze_host_mem_alloc_desc_t host_desc = {};
  host_desc.stype = ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC;

  host_desc.pNext = nullptr;
  host_desc.flags = flags;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemAllocHost(lzt::get_default_context(), &host_desc, size,
                           alignment, &memory));

  EXPECT_NE(nullptr, memory);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemFree(lzt::get_default_context(), memory));
}

INSTANTIATE_TEST_SUITE_P(
    TestHostMemParameterCombinations, zeDriverAllocHostMemTests,
    ::testing::Combine(
        ::testing::Values(0, ZE_HOST_MEM_ALLOC_FLAG_BIAS_CACHED,
                          ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED,
                          ZE_HOST_MEM_ALLOC_FLAG_BIAS_WRITE_COMBINED),
        lzt::memory_allocation_sizes, lzt::memory_allocation_alignments));

class zeDriverAllocHostMemAlignmentTests : public zeDriverAllocHostMemTests {
protected:
  void SetUp() override {
    const ze_host_mem_alloc_flag_t flags = std::get<0>(GetParam());
    size_ = std::get<1>(GetParam());
    alignment_ = std::get<2>(GetParam());
    memory_ = lzt::allocate_host_memory(size_, alignment_, flags, nullptr,
                                        lzt::get_default_context());
  }

  void TearDown() override { lzt::free_memory(memory_); }

  size_t size_;
  size_t alignment_;
  void *memory_ = nullptr;
};

TEST_P(zeDriverAllocHostMemAlignmentTests,
       GivenSizeAndAlignmentWhenAllocatingHostMemoryThenPointerIsAligned) {
  EXPECT_NE(nullptr, memory_);
  if (alignment_ != 0) {
    EXPECT_EQ(0u, reinterpret_cast<uintptr_t>(memory_) % alignment_);
  }
}

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocHostMemTestVarySizeAndAlignment,
    zeDriverAllocHostMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_small));

INSTANTIATE_TEST_SUITE_P(
    zeDriverAllocHostMemTestVarySizeAndLargeAlignment,
    zeDriverAllocHostMemAlignmentTests,
    ::testing::Combine(::testing::Values(0), lzt::memory_allocation_sizes,
                       lzt::memory_allocation_alignments_large));

class zeHostMemPropertiesTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(
    zeHostMemPropertiesTests,
    GivenValidMemoryPointerWhenQueryingAttributesOnHostMemoryAllocationThenSuccessIsReturned) {

  const size_t size = std::get<0>(GetParam());
  const size_t alignment = std::get<1>(GetParam());
  void *memory = lzt::allocate_host_memory(size, alignment);
  ze_memory_allocation_properties_t mem_properties = {};
  auto context = lzt::get_default_context();

  mem_properties.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  mem_properties.pNext = nullptr;
  lzt::get_mem_alloc_properties(context, memory, &mem_properties, nullptr);
  EXPECT_EQ(ZE_MEMORY_TYPE_HOST, mem_properties.type);

  lzt::free_memory(memory);
}

INSTANTIATE_TEST_SUITE_P(TestHostMemGetPropertiesParameterCombinations,
                         zeHostMemPropertiesTests,
                         ::testing::Combine(lzt::memory_allocation_sizes,
                                            lzt::memory_allocation_alignments));

class zeHostMemGetAddressRangeTests : public ::testing::Test {};

TEST_F(
    zeHostMemGetAddressRangeTests,
    GivenBasePointerWhenQueryingBaseAddressofHostMemoryAllocationThenSuccessIsReturned) {

  const size_t size = 1;
  const size_t alignment = 1;

  void *base = nullptr;

  void *memory = lzt::allocate_host_memory(size, alignment);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory, &base, nullptr));
  EXPECT_EQ(memory, base);

  lzt::free_memory(memory);
}

#ifdef ZE_MEMORY_FREE_POLICIES_EXT_NAME

bool check_ext_version() {
  auto ext_props = lzt::get_extension_properties(lzt::get_default_driver());
  uint32_t ext_version = 0;
  for (auto prop : ext_props) {
    if (strncmp(prop.name, ZE_MEMORY_FREE_POLICIES_EXT_NAME,
                ZE_MAX_EXTENSION_NAME) == 0) {
      ext_version = prop.version;
      break;
    }
  }
  if (ext_version == 0) {
    printf("ZE_MEMORY_FREE_POLICIES_EXT_NAME not found, not running test\n");
    return false;
  } else {
    printf("Extension version %d found\n", ext_version);
    return true;
  }
}

class zeMemFreeExtTests : public ::testing::Test {};

TEST_F(zeMemFreeExtTests, GetMemoryFreePolicyFlagsAndVerifySet) {
  if (!check_ext_version())
    GTEST_SKIP();

  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    ze_driver_properties_t properties = {};
    ze_driver_memory_free_ext_properties_t ext_properties = {};
    ext_properties.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
    properties.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
    properties.pNext = &ext_properties;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGetProperties(driver, &properties));
    EXPECT_GE(ext_properties.freePolicies, 0u);
    if (ext_properties.freePolicies > 0) {
      uint32_t valid_policy_flags_found = 0;
      if (ext_properties.freePolicies &
          ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE) {
        valid_policy_flags_found += 1;
      }
      if (ext_properties.freePolicies &
          ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE) {
        valid_policy_flags_found += 1;
      }
      EXPECT_NE(valid_policy_flags_found, 0u);
    }
  }
}

TEST_F(zeMemFreeExtTests, AllocateHostMemoryAndThenFreeWithBlockingFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_host_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

TEST_F(zeMemFreeExtTests,
       AllocateSharedMemoryAndThenFreeWithBlockingFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_shared_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

TEST_F(zeMemFreeExtTests,
       AllocateDeviceMemoryAndThenFreeWithBlockingFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_device_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

TEST_F(zeMemFreeExtTests, AllocateHostMemoryAndThenFreeWithDeferFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_host_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

TEST_F(zeMemFreeExtTests, AllocateSharedMemoryAndThenFreeWithDeferFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_shared_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

TEST_F(zeMemFreeExtTests, AllocateDeviceMemoryAndThenFreeWithDeferFreePolicy) {
  if (!check_ext_version())
    GTEST_SKIP();

  const size_t size = 1;
  const size_t alignment = 1;
  void *base = nullptr;

  void *memory = lzt::allocate_device_memory(size, alignment);

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;
  memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemFreeExt(lzt::get_default_context(), &memfreedesc, memory));
}

class zeMemFreeExtMultipleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, uint32_t, uint32_t, bool>> {};

TEST_P(
    zeMemFreeExtMultipleTests,
    AllocateMultipleDeviceMemoryAndThenFreeWithDeferFreePolicyWhileBuffersInUse) {
  if (!check_ext_version())
    GTEST_SKIP();
  const size_t num_buffers_max = 100;
  const size_t size = std::get<0>(GetParam());
  const size_t alignment = 1;
  void *base = nullptr;
  uint32_t num_buffers = std::get<1>(GetParam());
  if (num_buffers > num_buffers_max) {
    num_buffers = num_buffers_max;
  }
  uint32_t num_loops = std::get<2>(GetParam());
  const ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  const bool is_immediate = std::get<3>(GetParam());

  uint8_t pattern[num_buffers_max];
  uint8_t *dev_fill_buf[num_buffers_max];
  uint8_t *dev_copy_buf[num_buffers_max];
  uint8_t *host_verify_buf[num_buffers_max];

  ze_memory_free_ext_desc_t memfreedesc = {};
  memfreedesc.stype = ZE_STRUCTURE_TYPE_DRIVER_MEMORY_FREE_EXT_PROPERTIES;

  for (uint32_t type = 0; type < 2; type++) {
    if (type == 0) {
      memfreedesc.freePolicy =
          ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_BLOCKING_FREE;
    } else {
      memfreedesc.freePolicy = ZE_DRIVER_MEMORY_FREE_POLICY_EXT_FLAG_DEFER_FREE;
    }

    for (uint32_t count = 0; count < num_loops; count++) {
      const ze_context_handle_t context = lzt::create_context();
      auto bundle = lzt::create_command_bundle(
          context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
          ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

      for (uint32_t i = 0; i < num_buffers; i++) {
        dev_fill_buf[i] = static_cast<uint8_t *>(lzt::allocate_device_memory(
            size, alignment, 0, 0, device, context));
        EXPECT_NE(nullptr, dev_fill_buf[i]);
        dev_copy_buf[i] = static_cast<uint8_t *>(lzt::allocate_device_memory(
            size, alignment, 0, 0, device, context));
        EXPECT_NE(nullptr, dev_copy_buf[i]);
        pattern[i] = rand() & 0xff;
        host_verify_buf[i] = new uint8_t[size]();
        EXPECT_NE(nullptr, host_verify_buf[i]);
      }

      for (uint32_t i = 0; i < num_buffers; i++) {
        lzt::append_memory_fill(bundle.list, dev_fill_buf[i], &pattern[i], 1,
                                size, nullptr);
        lzt::append_barrier(bundle.list);
        lzt::append_memory_copy(bundle.list, dev_copy_buf[i], dev_fill_buf[i],
                                size);
        lzt::append_barrier(bundle.list);
        lzt::append_memory_copy(bundle.list, host_verify_buf[i],
                                dev_copy_buf[i], size);
        lzt::append_barrier(bundle.list);
      }

      lzt::close_command_list(bundle.list);
      if (!is_immediate) {
        lzt::execute_command_lists(bundle.queue, 1, &bundle.list, nullptr);
      }

      for (uint32_t i = 0; i < num_buffers; i++) {
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeMemFreeExt(context, &memfreedesc, dev_fill_buf[i]));
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeMemFreeExt(context, &memfreedesc, dev_copy_buf[i]));
      }

      if (is_immediate) {
        lzt::synchronize_command_list_host(bundle.list, UINT64_MAX);
      } else {
        lzt::synchronize(bundle.queue, UINT64_MAX);
      }

      for (uint32_t i = 0; i < num_buffers; i++) {
        bool failed = false;
        for (size_t j = 0; j < size; j++) {
          if (host_verify_buf[i][j] != pattern[i]) {
            failed = true;
            break;
          }
        }
        delete[] host_verify_buf[i];
        if (failed) {
          GTEST_FAIL() << "Verification failed";
          break;
        }
      }
      lzt::destroy_command_bundle(bundle);
      lzt::destroy_context(context);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    TestDeferAndBlockingPermuatations, zeMemFreeExtMultipleTests,
    ::testing::Combine(::testing::Values(500000, 1000000, 5000000, 10000000),
                       ::testing::Values(5, 10), ::testing::Values(100),
                       ::testing::Bool()));

#endif

class zeHostMemGetAddressRangeSizeTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(
    zeHostMemGetAddressRangeSizeTests,
    GivenSizePointerWhenQueryingSizeOfHostMemoryAllocationThenSuccessIsReturned) {

  const size_t size = std::get<0>(GetParam());
  const size_t alignment = std::get<1>(GetParam());

  size_t size_out;
  void *memory = lzt::allocate_host_memory(size, alignment);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetAddressRange(lzt::get_default_context(), memory, nullptr,
                                 &size_out));
  EXPECT_GE(size_out, size);

  lzt::free_memory(memory);
}

INSTANTIATE_TEST_SUITE_P(TestHostMemGetAddressRangeSizeTests,
                         zeHostMemGetAddressRangeSizeTests,
                         ::testing::Combine(lzt::memory_allocation_sizes,
                                            lzt::memory_allocation_alignments));

class zeHostMemGetAddressRangeParameterTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<size_t, size_t>> {};

TEST_P(
    zeHostMemGetAddressRangeParameterTests,
    GivenBasePointerAndSizeWhenQueryingBaseAddressOfHostMemoryAllocationThenSuccessIsReturned) {

  const size_t size = std::get<0>(GetParam());
  const size_t alignment = std::get<1>(GetParam());

  void *memory = lzt::allocate_host_memory(size, alignment);
  void *base = nullptr;
  size_t size_out;

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetAddressRange(lzt::get_default_context(),
                                                    memory, &base, &size_out));
  EXPECT_EQ(memory, base);

  if (size > 1) {
    uint8_t *mem_target = static_cast<uint8_t *>(memory) + (size - 1) / 2;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(), mem_target,
                                   &base, &size_out));
    EXPECT_EQ(memory, base);
    EXPECT_GE(size_out, size);

    mem_target = static_cast<uint8_t *>(memory) + (size - 1);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetAddressRange(lzt::get_default_context(), mem_target,
                                   &base, &size_out));
    EXPECT_EQ(memory, base);
    EXPECT_GE(size_out, size);
  }

  lzt::free_memory(memory);
}

INSTANTIATE_TEST_SUITE_P(zeHostMemGetAddressRangeParameterizedTests,
                         zeHostMemGetAddressRangeParameterTests,
                         ::testing::Combine(lzt::memory_allocation_sizes,
                                            lzt::memory_allocation_alignments));

class zeHostSystemMemoryHostTests : public ::testing::Test {
protected:
  zeHostSystemMemoryHostTests() { memory_ = new uint8_t[size_]; }
  ~zeHostSystemMemoryHostTests() { delete[] memory_; }
  const size_t size_ = 4 * 1024;
  uint8_t *memory_ = nullptr;
};

TEST_F(
    zeHostSystemMemoryHostTests,
    GivenHostSystemAllocationWhenWritingAndReadingOnHostThenCorrectDataIsRead) {
  lzt::write_data_pattern(memory_, size_, 1);
  lzt::validate_data_pattern(memory_, size_, 1);
}

class zeHostSystemMemoryDeviceTests : public ::testing::Test {
protected:
  void SetUp() override {
    auto mem_access_props = lzt::get_memory_access_properties(
        lzt::get_default_device(lzt::get_default_driver()));
    systemMemSupported = mem_access_props.sharedSystemAllocCapabilities &
                         ZE_MEMORY_ACCESS_CAP_FLAG_RW;
    memory_ = new uint8_t[size_];
  }
  void TearDown() override { delete[] memory_; }
  const size_t size_ = 4 * 1024;
  uint8_t *memory_ = nullptr;
  lzt::zeCommandBundle cmd_bundle_;
  bool systemMemSupported;

  void RunGivenHostSystemAllocationWhenAccessingMemoryOnDeviceTest(
      bool is_immediate);
  void
  RunGivenHostSystemAllocationWhenCopyingMemoryOnDeviceTest(bool is_immediate);
  void RunGivenHostSystemMemoryWhenSettingMemoryOnDeviceThenMemorySetCorrectly(
      bool is_immediate);
};

void zeHostSystemMemoryDeviceTests::
    RunGivenHostSystemAllocationWhenAccessingMemoryOnDeviceTest(
        bool is_immediate) {
  if (!systemMemSupported) {
    FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
              "system memory";
  }
  lzt::write_data_pattern(memory_, size_, 1);
  std::string module_name = "unified_mem_test.spv";
  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), module_name);
  std::string func_name = "unified_mem_test";

  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  arg.arg_size = sizeof(uint8_t *);
  arg.arg_value = &memory_;
  args.push_back(arg);
  arg.arg_size = sizeof(int);
  int size = static_cast<int>(size_);
  arg.arg_value = &size;
  args.push_back(arg);
  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name, 1, args, is_immediate);
  lzt::validate_data_pattern(memory_, size_, -1);
  lzt::destroy_module(module);
}

TEST_F(
    zeHostSystemMemoryDeviceTests,
    GivenHostSystemAllocationWhenAccessingMemoryOnDeviceThenCorrectDataIsRead) {
  RunGivenHostSystemAllocationWhenAccessingMemoryOnDeviceTest(false);
}

TEST_F(
    zeHostSystemMemoryDeviceTests,
    GivenHostSystemAllocationWhenAccessingMemoryOnDeviceOnImmediateCmdListThenCorrectDataIsRead) {
  RunGivenHostSystemAllocationWhenAccessingMemoryOnDeviceTest(true);
}

void zeHostSystemMemoryDeviceTests::
    RunGivenHostSystemAllocationWhenCopyingMemoryOnDeviceTest(
        bool is_immediate) {
  if (!systemMemSupported) {
    FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
              "system memory";
  }
  cmd_bundle_ = lzt::create_command_bundle(is_immediate);
  lzt::write_data_pattern(memory_, size_, 1);
  uint8_t *other_system_memory = new uint8_t[size_]();

  lzt::append_memory_copy(cmd_bundle_.list, other_system_memory, memory_, size_,
                          nullptr);
  lzt::append_barrier(cmd_bundle_.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle_.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_, UINT64_MAX);
  lzt::validate_data_pattern(other_system_memory, size_, 1);
  lzt::destroy_command_bundle(cmd_bundle_);
  delete[] other_system_memory;
}

TEST_F(
    zeHostSystemMemoryDeviceTests,
    GivenHostSystemAllocationWhenCopyingMemoryOnDeviceThenMemoryCopiedCorrectly) {
  RunGivenHostSystemAllocationWhenCopyingMemoryOnDeviceTest(false);
}

TEST_F(
    zeHostSystemMemoryDeviceTests,
    GivenHostSystemAllocationWhenCopyingMemoryOnDeviceOnImmediateCmdListThenMemoryCopiedCorrectly) {
  RunGivenHostSystemAllocationWhenCopyingMemoryOnDeviceTest(true);
}

void zeHostSystemMemoryDeviceTests::
    RunGivenHostSystemMemoryWhenSettingMemoryOnDeviceThenMemorySetCorrectly(
        bool is_immediate) {
  if (!systemMemSupported) {
    FAIL() << "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE - Device does not support "
              "system memory";
  }
  cmd_bundle_ = lzt::create_command_bundle(is_immediate);
  const uint8_t value = 0x55;
  lzt::write_data_pattern(memory_, size_, 1);
  lzt::append_memory_set(cmd_bundle_.list, memory_, &value, size_);
  lzt::append_barrier(cmd_bundle_.list, nullptr, 0, nullptr);
  lzt::close_command_list(cmd_bundle_.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_, UINT64_MAX);
  for (unsigned int ui = 0; ui < size_; ui++) {
    EXPECT_EQ(value, static_cast<uint8_t *>(memory_)[ui]);
  }
  lzt::destroy_command_bundle(cmd_bundle_);
}

TEST_F(zeHostSystemMemoryDeviceTests,
       GivenHostSystemMemoryWhenSettingMemoryOnDeviceThenMemorySetCorrectly) {
  RunGivenHostSystemMemoryWhenSettingMemoryOnDeviceThenMemorySetCorrectly(
      false);
}

TEST_F(
    zeHostSystemMemoryDeviceTests,
    GivenHostSystemMemoryWhenSettingMemoryOnDeviceOnImmediateCmdListThenMemorySetCorrectly) {
  RunGivenHostSystemMemoryWhenSettingMemoryOnDeviceThenMemorySetCorrectly(true);
}

} // namespace
