/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

TEST(zeDeviceGetTests,
     GivenZeroCountWhenRetrievingDevicesThenValidCountReturned) {
  lzt::get_ze_device_count();
}

TEST(zeDeviceGetTests,
     GivenValidCountWhenRetrievingDevicesThenNotNullDevicesAreReturned) {

  auto device_count = lzt::get_ze_device_count();

  ASSERT_GT(device_count, 0);

  auto devices = lzt::get_ze_devices(device_count);
  for (auto device : devices) {
    EXPECT_NE(nullptr, device);
  }
}

TEST(zeDeviceGetDevicePropertiesTests,
     GivenValidDeviceWhenRetrievingPropertiesThenValidPropertiesAreReturned) {

  auto devices = lzt::get_ze_devices();
  ze_structure_type_t stype;
  for (auto device : devices) {
    ze_device_properties_t properties1 =
        lzt::get_device_properties(device, ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
    ze_device_properties_t properties2 = lzt::get_device_properties(
        device, ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2);
    EXPECT_EQ(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, properties1.stype);
    EXPECT_LT(0, properties1.timerResolution);
    EXPECT_EQ(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2, properties2.stype);
    EXPECT_LT(0, properties2.timerResolution);
    EXPECT_NE(properties2.timerResolution, properties1.timerResolution);
    uint64_t tres1 = properties1.timerResolution;
    uint64_t tres2 = properties2.timerResolution;
    properties1 = lzt::get_device_properties(
        device, ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2);
    properties2 =
        lzt::get_device_properties(device, ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
    EXPECT_EQ(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2, properties1.stype);
    EXPECT_LT(0, properties1.timerResolution);
    EXPECT_EQ(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, properties2.stype);
    EXPECT_LT(0, properties2.timerResolution);
    EXPECT_NE(properties1.timerResolution, properties2.timerResolution);
    EXPECT_EQ(tres1, properties2.timerResolution);
    EXPECT_EQ(tres2, properties1.timerResolution);
  }
}

TEST(
    zeDeviceGetComputePropertiesTests,
    GivenValidDeviceWhenRetrievingComputePropertiesThenValidPropertiesAreReturned) {

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);
  for (auto device : devices) {
    ze_device_compute_properties_t properties =
        lzt::get_compute_properties(device);

    EXPECT_GT(properties.maxTotalGroupSize, 0);
    EXPECT_GT(properties.maxGroupSizeX, 0);
    EXPECT_GT(properties.maxGroupSizeY, 0);
    EXPECT_GT(properties.maxGroupSizeZ, 0);
    EXPECT_GT(properties.maxGroupCountX, 0);
    EXPECT_GT(properties.maxGroupCountY, 0);
    EXPECT_GT(properties.maxGroupCountZ, 0);
    EXPECT_GT(properties.maxSharedLocalMemory, 0);
    EXPECT_GT(properties.numSubGroupSizes, 0);
    for (uint32_t i = 0; i < properties.numSubGroupSizes; ++i) {
      EXPECT_NE(0, properties.subGroupSizes[i]);
    }
  }
}

TEST(
    zeDeviceGetMemoryPropertiesTests,
    GivenValidCountPointerWhenRetrievingMemoryPropertiesThenValidCountReturned) {
  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);
  for (auto device : devices) {
    lzt::get_memory_properties_count(device);
  }
}

TEST(
    zeDeviceGetMemoryPropertiesTests,
    GivenValidDeviceWhenRetrievingMemoryPropertiesThenValidPropertiesAreReturned) {

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  for (auto device : devices) {
    uint32_t count = lzt::get_memory_properties_count(device);
    uint32_t count_out = count;

    ASSERT_GT(count, 0) << "no memory properties found";
    std::vector<ze_device_memory_properties_t> properties =
        lzt::get_memory_properties(device);

    for (uint32_t i = 0; i < count_out; ++i) {
      EXPECT_EQ(count_out, count);
      EXPECT_GT(properties[i].maxBusWidth, 0);
      EXPECT_GT(properties[i].totalSize, 0);
    }
  }
}

TEST(
    zeDeviceGetExternalMemoryPropertiesTests,
    GivenValidDeviceWhenRetrievingExternalMemoryPropertiesThenValidPropertiesAreReturned) {

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  for (auto device : devices) {
    lzt::get_external_memory_properties(device);
  }
}

TEST(
    zeDeviceGetMemoryAccessTests,
    GivenValidDeviceWhenRetrievingMemoryAccessPropertiesThenValidPropertiesReturned) {
  auto devices = lzt::get_ze_devices();

  ASSERT_GT(devices.size(), 0);
  for (auto device : devices) {
    lzt::get_memory_access_properties(device);
  }
}

TEST(
    zeDeviceGetCachePropertiesTests,
    GivenValidDeviceWhenRetrievingCachePropertiesThenValidPropertiesAreReturned) {
  auto devices = lzt::get_ze_devices();

  ASSERT_GT(devices.size(), 0);
  for (auto device : devices) {
    auto properties = lzt::get_cache_properties(device);
  }
}

TEST(
    zeDeviceGetImagePropertiesTests,
    GivenValidDeviceWhenRetrievingImagePropertiesThenValidPropertiesAreReturned) {
  auto devices = lzt::get_ze_devices();

  ASSERT_GT(devices.size(), 0);
  for (auto device : devices) {
    ze_device_image_properties_t properties = lzt::get_image_properties(device);
  }
}

TEST(zeDeviceGetP2PPropertiesTests,
     GivenValidDevicesWhenRetrievingP2PThenValidPropertiesAreReturned) {
  auto drivers = lzt::get_all_driver_handles();

  ASSERT_GT(drivers.size(), 0)
      << "no drivers found for peer to peer device test";

  std::vector<ze_device_handle_t> devices;
  for (auto driver : drivers) {
    devices = lzt::get_ze_devices(driver);

    if (devices.size() >= 2)
      break;
  }
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }

  lzt::get_p2p_properties(devices[0], devices[1]);
}

TEST(zeDeviceCanAccessPeerTests,
     GivenValidDevicesWhenRetrievingCanAccessPropertyThenCapabilityIsReturned) {
  auto drivers = lzt::get_all_driver_handles();
  ASSERT_GT(drivers.size(), 0)
      << "no drivers found for peer to peer device test";

  std::vector<ze_device_handle_t> devices;
  for (auto driver : drivers) {
    devices = lzt::get_ze_devices(driver);

    if (devices.size() >= 1)
      break;
  }

  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }

  ze_bool_t a2b, b2a;
  a2b = lzt::can_access_peer(devices[0], devices[1]);
  b2a = lzt::can_access_peer(devices[1], devices[0]);

  EXPECT_EQ(a2b, b2a);
}

TEST(
    zeDeviceGetModulePropertiesTests,
    GivenValidDeviceWhenRetrievingModulePropertiesThenValidPropertiesReturned) {

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  for (auto device : devices) {
    auto properties = lzt::get_device_module_properties(device);

    LOG_DEBUG << "SPIR-V version supported "
              << ZE_MAJOR_VERSION(properties.spirvVersionSupported) << "."
              << ZE_MINOR_VERSION(properties.spirvVersionSupported);
    LOG_DEBUG << "nativeKernelSupported: " << properties.nativeKernelSupported;
    LOG_DEBUG << "16-bit Floating Point Supported: "
              << lzt::to_string(properties.fp16flags);
    LOG_DEBUG << "32-bit Floating Point Supported: "
              << lzt::to_string(properties.fp32flags);
    LOG_DEBUG << "64-bit Floating Point Supported: "
              << lzt::to_string(properties.fp64flags);
    LOG_DEBUG << "Max Argument Size: " << properties.maxArgumentsSize;
    LOG_DEBUG << "Print Buffer Size: " << properties.printfBufferSize;
  }

#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
  auto driver_extension_properties =
      lzt::get_extension_properties(lzt::get_default_driver());
  bool supports_kernel_hints_ext = false;
  for (auto &extension : driver_extension_properties) {
    if (!std::strcmp(ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME, extension.name)) {
      supports_kernel_hints_ext = true;
      break;
    }
  }

  if (!supports_kernel_hints_ext) {
    LOG_WARNING << "Kernel Hints Ext not supported";
    return;
  }

  for (auto device : devices) {
    auto hints = lzt::get_device_kernel_schedule_hints(device);
    LOG_DEBUG << "Scheduling Hint Flags: " << hints.schedulingHintFlags;
  }
#endif
}

typedef struct DeviceHandlesBySku_ {
  uint32_t vendorId;
  uint32_t deviceId;
  std::vector<ze_device_handle_t> deviceHandlesForSku;
} DeviceHandlesBySku_t;

// Return false if uuuid's are NOT equal.
bool areDeviceUuidsEqual(ze_device_uuid_t uuid1, ze_device_uuid_t uuid2) {
  if (std::memcmp(&uuid1, &uuid2, sizeof(ze_device_uuid_t))) {
    return false;
  }
  return true;
}

class DevicePropertiesTest : public ::testing::Test {
public:
  std::vector<DeviceHandlesBySku_t *> deviceHandlesAllSkus;

  DeviceHandlesBySku_t *findDeviceHandlesBySku(uint32_t vendorId,
                                               uint32_t deviceId);
  void addDeviceHandleBySku(uint32_t vendorId, uint32_t deviceId,
                            ze_device_handle_t handle);
  void populateDevicesBySku();
  void freeDevicesBySku();

  DevicePropertiesTest() { populateDevicesBySku(); }

  ~DevicePropertiesTest() { freeDevicesBySku(); }
};

DeviceHandlesBySku_t *
DevicePropertiesTest::findDeviceHandlesBySku(uint32_t vendorId,
                                             uint32_t deviceId) {
  DeviceHandlesBySku_t *foundSkuHandles = nullptr;

  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {
    if ((iterSkuHandles->vendorId == vendorId) &&
        (iterSkuHandles->deviceId == deviceId)) {
      foundSkuHandles = iterSkuHandles;
      break;
    }
  }
  return foundSkuHandles;
}

void DevicePropertiesTest::addDeviceHandleBySku(uint32_t vendorId,
                                                uint32_t deviceId,
                                                ze_device_handle_t handle) {
  DeviceHandlesBySku_t *skuHandles = findDeviceHandlesBySku(vendorId, deviceId);
  if (skuHandles == nullptr) {
    skuHandles = new DeviceHandlesBySku_t;
    skuHandles->vendorId = vendorId;
    skuHandles->deviceId = deviceId;
    DevicePropertiesTest::deviceHandlesAllSkus.push_back(skuHandles);
  }
  skuHandles->deviceHandlesForSku.push_back(handle);
}

void DevicePropertiesTest::populateDevicesBySku() {
  for (ze_device_handle_t deviceHandle : lzt::get_ze_devices()) {
    ze_device_properties_t deviceProperties =
        lzt::get_device_properties(deviceHandle);
    if (deviceProperties.type == ZE_DEVICE_TYPE_GPU) {
      addDeviceHandleBySku(deviceProperties.vendorId, deviceProperties.deviceId,
                           deviceHandle);
    }
  }
}

void DevicePropertiesTest::freeDevicesBySku() {
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {
    delete iterSkuHandles;
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatcheThenDevicePropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    ze_device_properties_t firstDeviceProperties =
        lzt::get_device_properties(firstDeviceHandle);
    EXPECT_FALSE(firstDeviceProperties.flags |
                 ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE);

    bool first_iteration = true;
    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      ze_device_properties_t iterDeviceProperties =
          lzt::get_device_properties(iterDeviceHandle);

      EXPECT_EQ(firstDeviceProperties.type, iterDeviceProperties.type);
      EXPECT_EQ(firstDeviceProperties.vendorId, iterDeviceProperties.vendorId);
      EXPECT_EQ(firstDeviceProperties.deviceId, iterDeviceProperties.deviceId);
      if (first_iteration) {
        EXPECT_TRUE(areDeviceUuidsEqual(firstDeviceProperties.uuid,
                                        iterDeviceProperties.uuid));
        first_iteration = false;
      } else {
        EXPECT_FALSE(areDeviceUuidsEqual(firstDeviceProperties.uuid,
                                         iterDeviceProperties.uuid));
      }
      EXPECT_FALSE(iterDeviceProperties.flags |
                   ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE);
      EXPECT_EQ(firstDeviceProperties.coreClockRate,
                iterDeviceProperties.coreClockRate);
      EXPECT_EQ(
          firstDeviceProperties.flags | ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING,
          iterDeviceProperties.flags | ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING);

      EXPECT_EQ(firstDeviceProperties.numThreadsPerEU,
                iterDeviceProperties.numThreadsPerEU);
      EXPECT_EQ(firstDeviceProperties.physicalEUSimdWidth,
                iterDeviceProperties.physicalEUSimdWidth);
      EXPECT_EQ(firstDeviceProperties.numEUsPerSubslice,
                iterDeviceProperties.numEUsPerSubslice);
      EXPECT_EQ(firstDeviceProperties.numSubslicesPerSlice,
                iterDeviceProperties.numSubslicesPerSlice);
      EXPECT_EQ(firstDeviceProperties.numSlices,
                iterDeviceProperties.numSlices);
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenComputePropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    ze_device_compute_properties_t firstDeviceProperties =
        lzt::get_compute_properties(firstDeviceHandle);

    EXPECT_GT(firstDeviceProperties.maxTotalGroupSize, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupSizeX, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupSizeY, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupSizeZ, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupCountX, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupCountY, 0);
    EXPECT_GT(firstDeviceProperties.maxGroupCountZ, 0);
    EXPECT_GT(firstDeviceProperties.maxSharedLocalMemory, 0);
    EXPECT_GT(firstDeviceProperties.numSubGroupSizes, 0);
    for (uint32_t i = 0; i < firstDeviceProperties.numSubGroupSizes; ++i) {
      EXPECT_NE(0, firstDeviceProperties.subGroupSizes[i]);
    }

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      ze_device_compute_properties_t iterDeviceProperties =
          lzt::get_compute_properties(iterDeviceHandle);

      EXPECT_EQ(firstDeviceProperties.maxTotalGroupSize,
                iterDeviceProperties.maxTotalGroupSize);
      EXPECT_EQ(firstDeviceProperties.maxGroupSizeX,
                iterDeviceProperties.maxGroupSizeX);
      EXPECT_EQ(firstDeviceProperties.maxGroupSizeY,
                iterDeviceProperties.maxGroupSizeY);
      EXPECT_EQ(firstDeviceProperties.maxGroupSizeZ,
                iterDeviceProperties.maxGroupSizeZ);
      EXPECT_EQ(firstDeviceProperties.maxGroupCountX,
                iterDeviceProperties.maxGroupCountX);
      EXPECT_EQ(firstDeviceProperties.maxGroupCountY,
                iterDeviceProperties.maxGroupCountY);
      EXPECT_EQ(firstDeviceProperties.maxGroupCountZ,
                iterDeviceProperties.maxGroupCountZ);
      EXPECT_EQ(firstDeviceProperties.maxSharedLocalMemory,
                iterDeviceProperties.maxSharedLocalMemory);
      EXPECT_EQ(firstDeviceProperties.numSubGroupSizes,
                iterDeviceProperties.numSubGroupSizes);
      for (uint32_t i = 0; i < firstDeviceProperties.numSubGroupSizes; ++i) {
        EXPECT_EQ(firstDeviceProperties.subGroupSizes[i],
                  iterDeviceProperties.subGroupSizes[i]);
        ;
      }
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenMemoryPropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    uint32_t firstDevicePropertiesCount =
        lzt::get_memory_properties_count(firstDeviceHandle);
    std::vector<ze_device_memory_properties_t> firstDeviceProperties =
        lzt::get_memory_properties(firstDeviceHandle);

    EXPECT_EQ(firstDevicePropertiesCount, firstDeviceProperties.size());

    for (uint32_t i = 0; i < firstDevicePropertiesCount; i++) {
      EXPECT_GT(firstDeviceProperties[i].maxClockRate, 0);
      EXPECT_GT(firstDeviceProperties[i].maxBusWidth, 0);
      EXPECT_GT(firstDeviceProperties[i].totalSize, 0);
    }

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      uint32_t iterDevicePropertiesCount =
          lzt::get_memory_properties_count(iterDeviceHandle);
      std::vector<ze_device_memory_properties_t> iterDeviceProperties =
          lzt::get_memory_properties(iterDeviceHandle);

      EXPECT_EQ(firstDevicePropertiesCount, iterDevicePropertiesCount);

      for (uint32_t i = 0; i < firstDevicePropertiesCount; i++) {
        EXPECT_EQ(iterDeviceProperties[i].maxClockRate,
                  firstDeviceProperties[i].maxClockRate);
        EXPECT_EQ(iterDeviceProperties[i].maxBusWidth,
                  firstDeviceProperties[i].maxBusWidth);
        EXPECT_EQ(iterDeviceProperties[i].totalSize,
                  firstDeviceProperties[i].totalSize);
      }
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenMemoryAccessPropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    ze_device_memory_access_properties_t firstDeviceProperties =
        lzt::get_memory_access_properties(firstDeviceHandle);

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      ze_device_memory_access_properties_t iterDeviceProperties =
          lzt::get_memory_access_properties(iterDeviceHandle);

      EXPECT_EQ(iterDeviceProperties.hostAllocCapabilities,
                firstDeviceProperties.hostAllocCapabilities);
      EXPECT_EQ(iterDeviceProperties.deviceAllocCapabilities,
                firstDeviceProperties.deviceAllocCapabilities);
      EXPECT_EQ(iterDeviceProperties.sharedSingleDeviceAllocCapabilities,
                firstDeviceProperties.sharedSingleDeviceAllocCapabilities);
      EXPECT_EQ(iterDeviceProperties.sharedCrossDeviceAllocCapabilities,
                firstDeviceProperties.sharedCrossDeviceAllocCapabilities);
      EXPECT_EQ(iterDeviceProperties.sharedSystemAllocCapabilities,
                firstDeviceProperties.sharedSystemAllocCapabilities);
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenCachePropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    auto firstDeviceProperties = lzt::get_cache_properties(firstDeviceHandle);

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      auto iterDeviceProperties = lzt::get_cache_properties(iterDeviceHandle);
      ASSERT_EQ(iterDeviceProperties.size(), firstDeviceProperties.size());
      for (int i = 0; i < iterDeviceProperties.size(); i++) {
        EXPECT_EQ(iterDeviceProperties[i].flags,
                  firstDeviceProperties[i].flags);
        EXPECT_EQ(iterDeviceProperties[i].cacheSize,
                  firstDeviceProperties[i].cacheSize);
      }
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenPeerAccessPropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();

    EXPECT_TRUE(lzt::can_access_peer(firstDeviceHandle, firstDeviceHandle));

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      ze_device_p2p_properties_t iterDeviceProperties =
          lzt::get_p2p_properties(firstDeviceHandle, iterDeviceHandle);
      ze_bool_t a2b, b2a;
      a2b = lzt::can_access_peer(firstDeviceHandle, iterDeviceHandle);
      b2a = lzt::can_access_peer(iterDeviceHandle, firstDeviceHandle);
      EXPECT_EQ(a2b, b2a);
    }
  }
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenSubDeviceCountsMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    uint32_t firstDeviceSubDeviceCount =
        lzt::get_ze_sub_device_count(firstDeviceHandle);

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      uint32_t iterDeviceSubDeviceCount =
          lzt::get_ze_sub_device_count(iterDeviceHandle);
      EXPECT_EQ(iterDeviceSubDeviceCount, firstDeviceSubDeviceCount);
    }
  }
}

// Return false if uuuid's are NOT equal.
bool areNativeKernelUuidsEqual(ze_native_kernel_uuid_t *uuid1,
                               ze_native_kernel_uuid_t *uuid2) {
  if (std::memcmp(uuid1->id, uuid2->id, ZE_MAX_NATIVE_KERNEL_UUID_SIZE)) {
    return false;
  }
  return true;
}

TEST_F(DevicePropertiesTest,
       GivenMultipleRootDevicesWhenSKUsMatchThenKernelPropertiesMatch) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  for (DeviceHandlesBySku_t *iterSkuHandles :
       DevicePropertiesTest::deviceHandlesAllSkus) {

    ze_device_handle_t firstDeviceHandle =
        iterSkuHandles->deviceHandlesForSku.front();
    auto firstDeviceProperties =
        lzt::get_device_module_properties(firstDeviceHandle);

    for (ze_device_handle_t iterDeviceHandle :
         iterSkuHandles->deviceHandlesForSku) {
      auto iterDeviceProperties =
          lzt::get_device_module_properties(iterDeviceHandle);

      EXPECT_EQ(iterDeviceProperties.spirvVersionSupported,
                firstDeviceProperties.spirvVersionSupported);

      EXPECT_TRUE(areNativeKernelUuidsEqual(
          &iterDeviceProperties.nativeKernelSupported,
          &firstDeviceProperties.nativeKernelSupported));

      EXPECT_EQ(iterDeviceProperties.fp16flags,
                firstDeviceProperties.fp16flags);

      EXPECT_EQ(iterDeviceProperties.fp32flags,
                firstDeviceProperties.fp32flags);

      EXPECT_EQ(iterDeviceProperties.fp64flags,
                firstDeviceProperties.fp64flags);

      EXPECT_EQ(iterDeviceProperties.maxArgumentsSize,
                firstDeviceProperties.maxArgumentsSize);

      EXPECT_EQ(iterDeviceProperties.printfBufferSize,
                firstDeviceProperties.printfBufferSize);
    }
  }
}

TEST(
    SubDeviceEnumeration,
    GivenRootDeviceWhenGettingSubdevicesThenAllSubdevicesHaveCorrectProperties) {

  for (auto device : lzt::get_ze_devices()) {
    std::vector<ze_device_handle_t> sub_devices =
        lzt::get_ze_sub_devices(device);

    auto root_dev_props = lzt::get_device_properties(device);

    for (auto sub_device : sub_devices) {
      ASSERT_NE(nullptr, sub_device);
      auto sub_dev_props = lzt::get_device_properties(sub_device);

      EXPECT_EQ(root_dev_props.type, sub_dev_props.type);
      EXPECT_EQ(root_dev_props.vendorId, sub_dev_props.vendorId);
    }
  }
}

TEST(
    TimestampsTest,
    GivenExecutedKernelWhenGettingGlobalTimestampsThenDeviceAndHostTimestampDurationsAreClose) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto context = lzt::create_context(driver);

  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto command_list = lzt::create_command_list(context, device, 0, 0);

  auto module = lzt::create_module(context, device, "module_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  auto kernel = lzt::create_function(module, "module_add_constant_2");

  auto timestamps = lzt::get_global_timestamps(device);

  LOG_DEBUG << "Timestamps0: " << std::get<0>(timestamps) << "......"
            << std::get<1>(timestamps) << std::endl;

  auto size = 10000000;
  auto buffer_a = lzt::allocate_shared_memory(size, 0, 0, 0, device, context);
  auto buffer_b = lzt::allocate_device_memory(size, 0, 0, 0, device, context);

  std::memset(buffer_a, 0, size);
  for (size_t i = 0; i < size; i++) {
    static_cast<uint8_t *>(buffer_a)[i] = (i & 0xFF);
  }

  const int addval = 3;
  lzt::set_argument_value(kernel, 0, sizeof(buffer_b), &buffer_b);
  lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);

  uint32_t group_size_x = 1;
  uint32_t group_size_y = 1;
  uint32_t group_size_z = 1;
  lzt::suggest_group_size(kernel, size, 1, 1, group_size_x, group_size_y,
                          group_size_z);
  lzt::set_group_size(kernel, group_size_x, 1, 1);
  ze_group_count_t group_count = {};
  group_count.groupCountX = size / group_size_x;
  group_count.groupCountY = 1;
  group_count.groupCountZ = 1;

  lzt::append_memory_copy(command_list, buffer_b, buffer_a, size);
  lzt::append_barrier(command_list);
  lzt::append_launch_function(command_list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::append_barrier(command_list);
  lzt::append_memory_copy(command_list, buffer_a, buffer_b, size);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  auto timestamps_1 = lzt::get_global_timestamps(device);

  LOG_DEBUG << "Timestamps1: " << std::get<0>(timestamps_1) << "......"
            << std::get<1>(timestamps_1) << std::endl;

  auto host_time_0 = std::get<0>(timestamps);
  auto host_time_1 = std::get<0>(timestamps_1);
  auto host_diff = host_time_1 - host_time_0;

  // calculate device elapsed time in ns
  auto device_time_0 = std::get<1>(timestamps);
  auto device_time_1 = std::get<1>(timestamps_1);
  auto device_properties = lzt::get_device_properties(device);
  uint64_t timestamp_max_val =
      ~(-1 << device_properties.kernelTimestampValidBits);
  uint64_t timestamp_freq = device_properties.timerResolution;

  auto device_diff_ns =
      (device_time_1 >= device_time_0)
          ? (device_time_1 - device_time_0) * (double)timestamp_freq
          : ((timestamp_max_val - device_time_0) + device_time_1 + 1) *
                (double)timestamp_freq;

  LOG_DEBUG << "host " << host_diff << "\n";
  LOG_DEBUG << "device  " << device_diff_ns << " ns \n";

  EXPECT_NE(host_time_0, host_time_1);
  EXPECT_NE(device_time_0, device_time_1);

  // validation
  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(buffer_a)[i],
              static_cast<uint8_t>((i & 0xFF) + addval));
  }

  // cleanup
  lzt::free_memory(context, buffer_a);
  lzt::free_memory(context, buffer_b);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}

} // namespace
