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

class zeDeviceMakeMemoryResidentTests : public ::testing::Test {
protected:
  void SetUp() override { memory_ = lzt::allocate_device_memory(size_); }

  void TearDown() override {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDriverFreeMem(lzt::get_default_driver(), memory_));
  }

  void *memory_ = nullptr;
  const size_t size_ = 1024;
};

TEST_F(zeDeviceMakeMemoryResidentTests,
       GivenDeviceMemoryWhenMakingMemoryResidentThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceMakeMemoryResident(
                lzt::zeDevice::get_instance()->get_device(), memory_, size_));
}

class zeDeviceEvictMemoryTests : public zeDeviceMakeMemoryResidentTests {};

TEST_F(
    zeDeviceEvictMemoryTests,
    GivenResidentDeviceMemoryWhenEvictingResidentMemoryThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceMakeMemoryResident(
                lzt::zeDevice::get_instance()->get_device(), memory_, size_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceEvictMemory(lzt::zeDevice::get_instance()->get_device(),
                                memory_, size_));
}

class zeDeviceMakeImageResidentTests : public lzt::zeImageCreateCommonTests {};

TEST_F(zeDeviceMakeImageResidentTests,
       GivenDeviceImageWhenMakingImageResidentThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceMakeImageResident(
                                   lzt::zeDevice::get_instance()->get_device(),
                                   img.dflt_device_image_));
}

class zeDeviceEvictImageTests : public zeDeviceMakeImageResidentTests {};

TEST_F(zeDeviceEvictImageTests,
       GivenResidentDeviceImageWhenEvictingResidentImageThenSuccessIsReturned) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceMakeImageResident(
                                   lzt::zeDevice::get_instance()->get_device(),
                                   img.dflt_device_image_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceEvictImage(lzt::zeDevice::get_instance()->get_device(),
                               img.dflt_device_image_));
}

} // namespace
