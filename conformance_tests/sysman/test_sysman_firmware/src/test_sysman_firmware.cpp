/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <fstream>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

uint32_t get_prop_length(char *prop) { return std::strlen(prop); }

class FirmwareTest : public lzt::SysmanCtsClass {};

TEST_F(
    FirmwareTest,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}
TEST_F(
    FirmwareTest,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenNotNullFirmwareHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(firmwareHandles.size(), count);
    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
    }
  }
}

TEST_F(
    FirmwareTest,
    GivenInvalidComponentCountWhenRetrievingSysmanFirmwareHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, actualCount);
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t testCount = actualCount + 1;
    firmwareHandles = lzt::get_firmware_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}
TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto properties = lzt::get_firmware_properties(firmwareHandle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(get_prop_length(properties.name), ZES_STRING_PROPERTY_SIZE);
      EXPECT_GT(get_prop_length(properties.name), 0);
      EXPECT_LT(get_prop_length(properties.version), ZES_STRING_PROPERTY_SIZE);
    }
  }
}

TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto propertiesInitial = lzt::get_firmware_properties(firmwareHandle);
      auto propertiesLater = lzt::get_firmware_properties(firmwareHandle);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_TRUE(0 ==
                  std::strcmp(reinterpret_cast<char *>(propertiesInitial.name),
                              reinterpret_cast<char *>(propertiesLater.name)));
      EXPECT_TRUE(
          0 == std::strcmp(reinterpret_cast<char *>(propertiesInitial.version),
                           reinterpret_cast<char *>(propertiesLater.version)));
      EXPECT_EQ(propertiesInitial.canControl, propertiesLater.canControl);
    }
  }
}

TEST_F(FirmwareTest,
       GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectUpdatedChecksum) {
  char *fwDirEnv = getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  if (nullptr == fwDirEnv) {
    FAIL() << "ZE_LZT_FIRMWARE_DIRECTORY is not set..";
  }
  std::vector<char> testFwImage;
  std::string fwDir(fwDirEnv);
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto propFw = lzt::get_firmware_properties(firmwareHandle);
      if (propFw.canControl == true) {
        std::string fwName(reinterpret_cast<char *>(propFw.name));
        std::string fwToLoad = fwDir + "/" + fwName;
        std::ifstream inFileStream(fwToLoad, std::ios::binary | std::ios::ate);
        ASSERT_EQ(true, inFileStream.is_open())
            << "unable to find firmware image " << fwName;
        testFwImage.resize(inFileStream.tellg());
        inFileStream.read(testFwImage.data(), testFwImage.size());
        EXPECT_TRUE(false) << "flashing firmware: " << fwName;
        lzt::flash_firmware(firmwareHandle,
                            static_cast<void *>(testFwImage.data()),
                            testFwImage.size());
      }
    }
  }
}

} // namespace
