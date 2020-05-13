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

#include <level_zero/ze_api.h>

namespace {

uint32_t get_prop_length(int8_t *prop) {
  return std::strlen(reinterpret_cast<char *>(prop));
}

uint32_t read_cksum_file(std::string &cksumFile) {
  std::ifstream fs;
  uint32_t val = 0;
  fs.open(cksumFile.c_str());
  if (fs.fail()) {
    return -1;
  }
  fs >> val;
  if (fs.fail()) {
    fs.close();
    return -1;
  }
  fs.close();
  return val;
}

class FirmwareTest : public lzt::SysmanCtsClass {};

TEST_F(
    FirmwareTest,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
  }
}
TEST_F(
    FirmwareTest,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenNotNullFirmwareHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
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
    uint32_t testCount = actualCount + 1;
    firmwareHandles = lzt::get_firmware_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}
TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto properties = lzt::get_firmware_properties(firmwareHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      }
      EXPECT_LT(get_prop_length(properties.name), ZET_STRING_PROPERTY_SIZE);
      EXPECT_GT(get_prop_length(properties.name), 0);
      EXPECT_LT(get_prop_length(properties.version), ZET_STRING_PROPERTY_SIZE);
    }
  }
}

TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto propertiesInitial = lzt::get_firmware_properties(firmwareHandle);
      auto propertiesLater = lzt::get_firmware_properties(firmwareHandle);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
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
TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwareChecksumThenExpectValidIntegerType) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      uint32_t cksum = lzt::get_firmware_checksum(firmwareHandle);
      EXPECT_GE(cksum, 0);
      EXPECT_LE(cksum, UINT32_MAX);
    }
  }
}

TEST_F(FirmwareTest,
       GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectUpdatedChecksum) {
  ASSERT_EQ(nullptr, getenv("ZE_LZT_FIRMWARE_DIRECTORY"))
      << "unable to find directory " << getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  std::vector<char> testFwImage;
  std::string fwDir = getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmwareHandles = lzt::get_firmware_handles(device, count);
    for (auto firmwareHandle : firmwareHandles) {
      ASSERT_NE(nullptr, firmwareHandle);
      auto propFw = lzt::get_firmware_properties(firmwareHandle);
      if (propFw.canControl == true) {
        std::string fwName(reinterpret_cast<char *>(propFw.name));
        std::string fwToLoad = fwDir + "/" + fwName;
        std::ifstream inFileStream(fwToLoad, std::ios::binary | std::ios::ate);
        ASSERT_EQ(true, inFileStream.is_open())
            << "unable to find firnware image " << fwName;
        testFwImage.resize(inFileStream.tellg());
        inFileStream.read(testFwImage.data(), testFwImage.size());
        std::string fwToLoadCkSumFile = fwDir + "/" + fwName + ".cksum";
        uint32_t fwToLoadCkSum = read_cksum_file(fwToLoadCkSumFile);
        uint32_t originalFwImageCkSum =
            lzt::get_firmware_checksum(firmwareHandle);
        EXPECT_TRUE(false) << "flashing firmware: " << fwName;
        lzt::flash_firmware(firmwareHandle,
                            static_cast<void *>(testFwImage.data()),
                            testFwImage.size());

        EXPECT_EQ(fwToLoadCkSum, lzt::get_firmware_checksum(firmwareHandle));
      }
    }
  }
}

} // namespace
