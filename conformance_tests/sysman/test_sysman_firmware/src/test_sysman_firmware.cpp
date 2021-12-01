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
    auto firmware_handles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(firmware_handles.size(), count);
    for (auto firmware_handle : firmware_handles) {
      ASSERT_NE(nullptr, firmware_handle);
    }
  }
}

TEST_F(
    FirmwareTest,
    GivenInvalidComponentCountWhenRetrievingSysmanFirmwareHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    auto firmware_handles = lzt::get_firmware_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    firmware_handles = lzt::get_firmware_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}
TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto firmware_handles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmware_handle : firmware_handles) {
      ASSERT_NE(nullptr, firmware_handle);
      auto properties = lzt::get_firmware_properties(firmware_handle);
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
    auto firmware_handles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmware_handle : firmware_handles) {
      ASSERT_NE(nullptr, firmware_handle);
      auto properties_initial = lzt::get_firmware_properties(firmware_handle);
      auto properties_later = lzt::get_firmware_properties(firmware_handle);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_TRUE(0 ==
                  std::strcmp(reinterpret_cast<char *>(properties_initial.name),
                              reinterpret_cast<char *>(properties_later.name)));
      EXPECT_TRUE(
          0 == std::strcmp(reinterpret_cast<char *>(properties_initial.version),
                           reinterpret_cast<char *>(properties_later.version)));
      EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
    }
  }
}

TEST_F(
    FirmwareTest,
    GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectFirmwareFlashingSuccess) {
  auto fwDirEnv = getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  if (nullptr == fwDirEnv) {
    SUCCEED();
    LOG_INFO << "Skipping test as ZE_LZT_FIRMWARE_DIRECTORY  not set";
    return;
  }
  std::vector<char> testFwImage;
  std::string fwDir(fwDirEnv);
  for (auto device : devices) {
    uint32_t count = 0;
    auto firmware_handles = lzt::get_firmware_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto firmware_handle : firmware_handles) {
      ASSERT_NE(nullptr, firmware_handle);
      auto propFw = lzt::get_firmware_properties(firmware_handle);
      if (propFw.canControl == true) {
        std::string fwName(reinterpret_cast<char *>(propFw.name));
        std::string fwToLoad = fwDir + "/" + fwName + ".bin";
        std::ifstream inFileStream(fwToLoad, std::ios::binary | std::ios::ate);
        if (!inFileStream.is_open()) {
          SUCCEED();
          LOG_INFO << "Skipping test as firmware image not found";
          return;
        }
        testFwImage.resize(inFileStream.tellg());
        inFileStream.seekg(0, inFileStream.beg);
        inFileStream.read(testFwImage.data(), testFwImage.size());
        lzt::flash_firmware(firmware_handle,
                            static_cast<void *>(testFwImage.data()),
                            testFwImage.size());
      }
    }
  }
}

} // namespace
