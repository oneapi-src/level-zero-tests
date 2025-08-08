/*
 *
 * Copyright (C) 2020-2024 Intel Corporation
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
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

uint32_t get_prop_length(char *prop) { return std::strlen(prop); }

#ifdef USE_ZESINIT
class FirmwareZesTest : public lzt::ZesSysmanCtsClass {
public:
  bool is_firmware_supported = false;
};
#define FIRMWARE_TEST FirmwareZesTest
#else // USE_ZESINIT
class FirmwareTest : public lzt::SysmanCtsClass {
public:
  bool is_firmware_supported = false;
};
#define FIRMWARE_TEST FirmwareTest
#endif // USE_ZESINIT

LZT_TEST_F(
    FIRMWARE_TEST,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firmware handles are available on this device! ";
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    FIRMWARE_TEST,
    GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenNotNullFirmwareHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firmware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, count);
      ASSERT_EQ(firmware_handles.size(), count);
      for (auto firmware_handle : firmware_handles) {
        ASSERT_NE(nullptr, firmware_handle);
      }
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    FIRMWARE_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanFirmwareHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    actual_count = lzt::get_firmware_handle_count(device);
    if (actual_count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firmware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, actual_count);
      uint32_t test_count = actual_count + 1;
      firmware_handles = lzt::get_firmware_handles(device, test_count);
      EXPECT_EQ(test_count, actual_count);
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}
LZT_TEST_F(
    FIRMWARE_TEST,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firmware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, count);
      for (auto firmware_handle : firmware_handles) {
        ASSERT_NE(nullptr, firmware_handle);
        auto properties = lzt::get_firmware_properties(firmware_handle);
        if (properties.onSubdevice) {
          EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
        }
        EXPECT_LT(get_prop_length(properties.name), ZES_STRING_PROPERTY_SIZE);
        EXPECT_GT(get_prop_length(properties.name), 0);
        EXPECT_LT(get_prop_length(properties.version),
                  ZES_STRING_PROPERTY_SIZE);
      }
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    FIRMWARE_TEST,
    GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firmware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, count);
      for (auto firmware_handle : firmware_handles) {
        ASSERT_NE(nullptr, firmware_handle);
        auto properties_initial = lzt::get_firmware_properties(firmware_handle);
        auto properties_later = lzt::get_firmware_properties(firmware_handle);
        EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
        if (properties_initial.onSubdevice && properties_later.onSubdevice) {
          EXPECT_EQ(properties_initial.subdeviceId,
                    properties_later.subdeviceId);
        }
        EXPECT_TRUE(
            0 == std::strcmp(reinterpret_cast<char *>(properties_initial.name),
                             reinterpret_cast<char *>(properties_later.name)));
        EXPECT_TRUE(
            0 ==
            std::strcmp(reinterpret_cast<char *>(properties_initial.version),
                        reinterpret_cast<char *>(properties_later.version)));
        EXPECT_EQ(properties_initial.canControl, properties_later.canControl);
      }
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}

LZT_TEST_F(
    FIRMWARE_TEST,
    GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectFirmwareFlashingSuccess) {
  auto fwDirEnv = getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  if (nullptr == fwDirEnv) {
    LOG_INFO << "Skipping test as ZE_LZT_FIRMWARE_DIRECTORY  not set";
    GTEST_SKIP();
  }
  std::vector<char> testFwImage;
  std::string fwDir(fwDirEnv);
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_firmware_handle_count(device);
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firnware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, count);
      for (auto firmware_handle : firmware_handles) {
        ASSERT_NE(nullptr, firmware_handle);
        auto propFw = lzt::get_firmware_properties(firmware_handle);
        if (propFw.canControl == true) {
          std::string fwName(reinterpret_cast<char *>(propFw.name));
          std::string fwToLoad = fwDir + "/" + fwName + ".bin";
          std::ifstream inFileStream(fwToLoad,
                                     std::ios::binary | std::ios::ate);
          if (!inFileStream.is_open()) {
            LOG_INFO << "Skipping test as firmware image not found";
            GTEST_SKIP();
          }
          testFwImage.resize(inFileStream.tellg());
          inFileStream.seekg(0, inFileStream.beg);
          inFileStream.read(testFwImage.data(), testFwImage.size());
          lzt::flash_firmware(firmware_handle,
                              static_cast<void *>(testFwImage.data()),
                              testFwImage.size());
        }
      }
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}

void flash_firmware(zes_firmware_handle_t firmware_handle, std::string fw_dir) {
  std::vector<char> test_fw_image;
  ASSERT_NE(nullptr, firmware_handle);
  auto prop_fw = lzt::get_firmware_properties(firmware_handle);
  if (prop_fw.canControl == true) {
    std::string fw_name(reinterpret_cast<char *>(prop_fw.name));
    std::string fw_to_load = fw_dir + "/" + fw_name + ".bin";
    std::ifstream in_filestream(fw_to_load, std::ios::binary | std::ios::ate);
    if (!in_filestream.is_open()) {
      LOG_INFO << "Skipping test as firmware image not found";
      GTEST_SKIP();
    }
    test_fw_image.resize(in_filestream.tellg());
    in_filestream.seekg(0, in_filestream.beg);
    in_filestream.read(test_fw_image.data(), test_fw_image.size());
    lzt::flash_firmware(firmware_handle,
                        static_cast<void *>(test_fw_image.data()),
                        test_fw_image.size());
  } else {
    LOG_INFO
        << "Skipping test as canControl is set to false in firmware properties";
    GTEST_SKIP();
  }
}

void track_firmware_flash(zes_firmware_handle_t firmware_handle) {
  uint32_t progress_percent = 0;
  uint32_t prev_progress = 0;
  std::chrono::time_point<std::chrono::system_clock> start =
      std::chrono::system_clock::now();
  bool flash_complete = false;

  while (!flash_complete) {

    lzt::track_firmware_flash(firmware_handle, &progress_percent);
    EXPECT_GE(progress_percent, 0);
    EXPECT_LE(progress_percent, 100);

    if (progress_percent == 100) {
      flash_complete = true;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::chrono::duration<double> elapsed_seconds =
          std::chrono::system_clock::now() - start;
      if (elapsed_seconds >= std::chrono::seconds{120}) {
        if (progress_percent == prev_progress) {
          FAIL() << "TimeOut of 2 minutes elapsed while waiting for progress "
                    "to get update";
        } else {
          start = std::chrono::system_clock::now();
          prev_progress = progress_percent;
        }
      }
    }
  }
}

LZT_TEST_F(
    FIRMWARE_TEST,
    GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectFlashProgressGetsUpdated) {
  auto fw_dir_env = getenv("ZE_LZT_FIRMWARE_DIRECTORY");
  if (nullptr == fw_dir_env) {
    LOG_INFO << "Skipping test as ZE_LZT_FIRMWARE_DIRECTORY  not set";
    GTEST_SKIP();
  }

  std::string fw_dir(fw_dir_env);
  for (auto device : devices) {
    uint32_t count = 0;
    if (count > 0) {
      is_firmware_supported = true;
      LOG_INFO << "Firnware handles are available on this device! ";
      auto firmware_handles = lzt::get_firmware_handles(device, count);
      for (auto firmware_handle : firmware_handles) {
        std::thread firmware_flasher(flash_firmware, firmware_handle, fw_dir);
        std::thread progress_tracker(track_firmware_flash, firmware_handle);
        firmware_flasher.join();
        progress_tracker.join();
      }
    } else {
      LOG_INFO << "No firmware handles found for this device! ";
    }
  }
  if (!is_firmware_supported) {
    FAIL() << "No firmware handles found on any of the devices! ";
  }
}

} // namespace
