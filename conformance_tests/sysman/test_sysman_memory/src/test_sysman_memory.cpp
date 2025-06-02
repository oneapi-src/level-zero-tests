/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include <condition_variable>
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

#ifdef USE_ZESINIT
class MemoryModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define MEMORY_TEST MemoryModuleZesTest
#else // USE_ZESINIT
class MemoryModuleTest : public lzt::SysmanCtsClass {};
#define MEMORY_TEST MemoryModuleTest
#endif // USE_ZESINIT

TEST_F(
    MEMORY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullMemoryHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(mem_handles.size(), count);
    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_mem_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_mem_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles_initial = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles_initial) {
      EXPECT_NE(nullptr, mem_handle);
    }

    count = 0;
    auto mem_handles_later = lzt::get_mem_handles(device, count);
    for (auto mem_handle : mem_handles_later) {
      EXPECT_NE(nullptr, mem_handle);
    }
    EXPECT_EQ(mem_handles_initial, mem_handles_later);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(properties.physicalSize, UINT64_MAX);
      EXPECT_GE(properties.location, ZES_MEM_LOC_SYSTEM);
      EXPECT_LE(properties.location, ZES_MEM_LOC_DEVICE);
      EXPECT_LE(properties.busWidth, INT32_MAX);
      EXPECT_GE(properties.busWidth, -1);
      EXPECT_NE(properties.busWidth, 0);
      EXPECT_LE(properties.numChannels, INT32_MAX);
      EXPECT_GE(properties.numChannels, -1);
      EXPECT_NE(properties.numChannels, 0);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
      auto properties_initial = lzt::get_mem_properties(mem_handle);
      auto properties_later = lzt::get_mem_properties(mem_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.physicalSize, properties_later.physicalSize);
      EXPECT_EQ(properties_initial.location, properties_later.location);
      EXPECT_EQ(properties_initial.busWidth, properties_later.busWidth);
      EXPECT_EQ(properties_initial.numChannels, properties_later.numChannels);
    }
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleAndWorkloadWhenRetrievingMemBandWidthThenBandWidthCountersAreValidAndIncreaseAfterWorkload) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        lzt::get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    device = core_device;
#endif // USE_ZESINIT
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetProperties(device, &deviceProperties));
    if (deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
      std::cout << "test subdevice id " << deviceProperties.subdeviceId;
    } else {
      std::cout << "test device is a root device" << std::endl;
    }

    // Allocate device memory for workload
    const size_t buffer_size = 32 * 1024 * 1024;
    ze_device_mem_alloc_desc_t device_desc = {};
    device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    device_desc.ordinal = 0;
    device_desc.flags = 0;

    void *src_ptr = lzt::allocate_device_memory(buffer_size, 1, 0, device,
                                                lzt::get_default_context());
    void *dst_ptr = lzt::allocate_device_memory(buffer_size, 1, 0, device,
                                                lzt::get_default_context());

    ASSERT_NE(src_ptr, nullptr);
    ASSERT_NE(dst_ptr, nullptr);

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);

      // Get initial bandwidth counters
      auto bandwidth_before = lzt::get_mem_bandwidth(mem_handle);
      EXPECT_LT(bandwidth_before.readCounter, UINT64_MAX);
      EXPECT_LT(bandwidth_before.writeCounter, UINT64_MAX);
      EXPECT_LT(bandwidth_before.maxBandwidth, UINT64_MAX);
      EXPECT_LT(bandwidth_before.timestamp, UINT64_MAX);
      // Run the workload
      ze_result_t result = copy_workload(device, &device_desc, src_ptr, dst_ptr,
                                         static_cast<int32_t>(buffer_size));
      EXPECT_EQ(result, ZE_RESULT_SUCCESS);

      // Get bandwidth counters after workload
      auto bandwidth_after = lzt::get_mem_bandwidth(mem_handle);

      // Validate that read/write counters have increased after workload
      EXPECT_GE(bandwidth_after.readCounter, bandwidth_before.readCounter)
          << "Read counter did not increase after workload";
      EXPECT_GE(bandwidth_after.writeCounter, bandwidth_before.writeCounter)
          << "Write counter did not increase after workload";
      EXPECT_GE(bandwidth_after.maxBandwidth, bandwidth_before.maxBandwidth)
          << "Max bandwidth did not increase after workload";

      auto percentage_bandwidth =
          1000000 *
          ((bandwidth_after.readCounter - bandwidth_before.readCounter) +
           (bandwidth_after.writeCounter - bandwidth_before.writeCounter)) /
          (bandwidth_after.maxBandwidth *
           (bandwidth_after.timestamp - bandwidth_before.timestamp));
      // Validate that percentage bandwidth is greater than zero
      LOG_INFO << "Percentage Bandwidth: " << percentage_bandwidth << "%";
      EXPECT_GT(percentage_bandwidth, 0.0)
          << "Percentage bandwidth is not greater than zero";
      EXPECT_LT(percentage_bandwidth, 100.0);
    }
    // Free device memory
    lzt::free_memory(src_ptr);
    lzt::free_memory(dst_ptr);
  }
}

TEST_F(MEMORY_TEST,
       GivenValidMemHandleWhenRetrievingMemStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto state = lzt::get_mem_state(mem_handle);
      EXPECT_GE(state.health, ZES_MEM_HEALTH_UNKNOWN);
      EXPECT_LE(state.health, ZES_MEM_HEALTH_REPLACE);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.physicalSize != 0) {
        EXPECT_LE(state.size, properties.physicalSize);
      }
      EXPECT_LE(state.free, state.size);
    }
  }
}

uint64_t get_free_memory_state(ze_device_handle_t device) {
  uint32_t count = 0;
  uint64_t total_free_memory = 0;
  uint64_t total_alloc = 0;

  std::vector<zes_mem_handle_t> mem_handles =
      lzt::get_mem_handles(device, count);
  if (count == 0) {
    std::cout << "No handles found: "
              << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  for (zes_mem_handle_t mem_handle : mem_handles) {
    auto state = lzt::get_mem_state(mem_handle);
    total_free_memory += state.free;
    total_alloc += state.size;
  }

  return total_free_memory;
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemHandleWhenAllocatingMemoryUptoMaxCapacityThenOutOfDeviceMemoryErrorIsReturned) {

  for (ze_device_handle_t device : devices) {
    ze_device_mem_alloc_desc_t device_desc = {};
    device_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
    device_desc.ordinal = 0;
    device_desc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED;
    device_desc.pNext = nullptr;

    std::vector<void *> alloc_list;
    ze_result_t result = ZE_RESULT_SUCCESS;

#ifdef USE_ZESINIT
    auto sysman_device_properties = lzt::get_sysman_device_properties(device);
    ze_device_handle_t core_device =
        lzt::get_core_device_by_uuid(sysman_device_properties.core.uuid.id);
    EXPECT_NE(core_device, nullptr);
    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    result = zeDeviceGetProperties(core_device, &deviceProperties);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    std::cout << "test device name " << deviceProperties.name << " uuid "
              << lzt::to_string(deviceProperties.uuid);
    device = core_device;
#else  // USE_ZESINIT
    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    result = zeDeviceGetProperties(device, &deviceProperties);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    std::cout << "test device name " << deviceProperties.name << " uuid "
              << lzt::to_string(deviceProperties.uuid);
#endif // USE_ZESINIT

    if (deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
      std::cout << "test subdevice id " << deviceProperties.subdeviceId;
    } else {
      std::cout << "test device is a root device" << std::endl;
    }

    uint64_t max_alloc_size = deviceProperties.maxMemAllocSize;
    uint64_t alloc_size = max_alloc_size;
    void *local_mem = nullptr;
    result = zeMemAllocDevice(lzt::get_default_context(), &device_desc,
                              alloc_size, 1, device, &local_mem);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);

    result = zeContextMakeMemoryResident(lzt::get_default_context(), device,
                                         local_mem, alloc_size);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);

    result = copy_workload(device, &device_desc, local_mem,
                           static_cast<int *>(local_mem) + alloc_size / 2,
                           alloc_size / 2);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);

    result = zeMemFree(lzt::get_default_context(), local_mem);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);

    uint32_t count = 0;
    do {
      local_mem = nullptr;
      alloc_size = max_alloc_size;
      result = zeMemAllocDevice(lzt::get_default_context(), &device_desc,
                                alloc_size, 1, device, &local_mem);
      if (result != ZE_RESULT_SUCCESS) {
        break;
      }

      alloc_list.push_back(local_mem);
      result = zeContextMakeMemoryResident(lzt::get_default_context(), device,
                                           local_mem, alloc_size);
      if (result != ZE_RESULT_SUCCESS) {
        break;
      }

      result = copy_workload(device, &device_desc, local_mem,
                             static_cast<uint8_t *>(local_mem) + alloc_size / 2,
                             alloc_size / 2);
      count++;
    } while (result != ZE_RESULT_SUCCESS || count < 10);

    EXPECT_EQ(ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY, result);

    for (int i = 0; i < alloc_list.size(); i++) {
      result = zeMemFree(lzt::get_default_context(), alloc_list[i]);
      EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    }
    alloc_list.clear();
  }
}

void getMemoryState(ze_device_handle_t device) {
  uint32_t count = 0;
  std::vector<zes_mem_handle_t> mem_handles =
      lzt::get_mem_handles(device, count);
  if (count == 0) {
    FAIL() << "No handles found: "
           << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  for (auto mem_handle : mem_handles) {
    ASSERT_NE(nullptr, mem_handle);
    lzt::get_mem_state(mem_handle);
  }
}

void getRasState(ze_device_handle_t device) {
  uint32_t count = 0;
  std::vector<zes_ras_handle_t> ras_handles =
      lzt::get_ras_handles(device, count);
  if (count == 0) {
    FAIL() << "No handles found: "
           << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
  }
  for (auto ras_handle : ras_handles) {
    ASSERT_NE(nullptr, ras_handle);
    ze_bool_t clear = 0;
    lzt::get_ras_state(ras_handle, clear);
  }
}

TEST_F(
    MEMORY_TEST,
    GivenValidMemoryAndRasHandlesWhenGettingMemoryGetStateAndRasGetStateFromDifferentThreadsThenExpectBothToReturnSucess) {
  for (auto device : devices) {
    std::thread rasThread(getRasState, device);
    std::thread memoryThread(getMemoryState, device);
    rasThread.join();
    memoryThread.join();
  }
}
} // namespace
