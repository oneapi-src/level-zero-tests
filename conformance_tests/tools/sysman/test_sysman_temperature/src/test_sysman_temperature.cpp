#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class TempModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    TempModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    lzt::get_temp_handle_count(device);
  }
}

TEST_F(
    TempModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullTempHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    ASSERT_EQ(tempHandles.size(), count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_temp_handles(device, actualCount);
    uint32_t testCount = actualCount + 1;
    lzt::get_temp_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    TempModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarTempHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandlesInitial = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandlesInitial) {
      ASSERT_NE(nullptr, tempHandle);
    }

    count = 0;
    auto tempHandlesLater = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandlesLater) {
      ASSERT_NE(nullptr, tempHandle);
    }
    EXPECT_EQ(tempHandlesInitial, tempHandlesLater);
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto properties = lzt::get_temp_properties(tempHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.subdeviceId);
      }
      EXPECT_GE(properties.type, ZET_TEMP_SENSORS_GLOBAL);
      EXPECT_LE(properties.type, ZET_TEMP_SENSORS_MEMORY);
    }
  }
}
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto propertiesInitial = lzt::get_temp_properties(tempHandle);
      auto propertiesLater = lzt::get_temp_properties(tempHandle);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(propertiesInitial.isThreshold1Supported, propertiesLater.isThreshold1Supported);
      EXPECT_EQ(propertiesInitial.isCriticalTempSupported, propertiesLater.isCriticalTempSupported);
      EXPECT_EQ(propertiesInitial.isThreshold2Supported, propertiesLater.isThreshold2Supported);
    }
  }
}

TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenRetrievingTempConfigurationThenValidTempConfigurationIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      auto config = lzt::get_temp_configuration(tempHandle);
      auto properties = lzt::get_temp_properties(tempHandle);
      if(config.enableCritical == true){
        ASSERT_EQ(properties.isCriticalTempSupported, true);
      }
      if((config.threshold1.enableLowToHigh == true) || (config.threshold1.enableHighToLow == true)){
        ASSERT_EQ(properties.isThreshold1Supported, true);
	EXPECT_GT(config.threshold1.threshold, 0);
      }
      if((config.threshold2.enableLowToHigh == true) || (config.threshold2.enableHighToLow == true)){
        ASSERT_EQ(properties.isThreshold2Supported, true);
        EXPECT_GT(config.threshold2.threshold, 0);
      }
      EXPECT_LT(config.processId, UINT32_MAX);
        
      }
    }
  }
TEST_F(
    TempModuleTest,
    GivenValidTempHandleWhenSettingTempConfigurationThenExpectzetSysmanTemperatureSetConfigFollowedByzetSysmanTemperatureGetConfigToMatch) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      uint32_t pCount = 0;
      auto freqHandles = lzt::get_freq_handles(device, pCount);
      for (auto freqHandle : freqHandles) {
        EXPECT_NE(nullptr, freqHandle);
        auto properties = lzt::get_temp_properties(tempHandle);
        auto initial_config = lzt::get_temp_configuration(tempHandle);
        zet_temp_config_t set_config;
        set_config.enableCritical = true;
        set_config.threshold1.enableLowToHigh = true;
        set_config.threshold1.enableHighToLow = false;
        double tMax = 0;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFrequencyOcGetTjMax(freqHandle, &tMax));
        set_config.threshold1.threshold = tMax;
        set_config.threshold2.enableLowToHigh = true;
        set_config.threshold2.enableHighToLow = false;
        set_config.threshold2.threshold = tMax;
        lzt::set_temp_configuration(tempHandle, set_config);
        auto get_config = lzt::get_temp_configuration(tempHandle);
        EXPECT_EQ(get_config.enableCritical, set_config.enableCritical);
        EXPECT_EQ(get_config.threshold1.enableLowToHigh, set_config.threshold1.enableLowToHigh);
        EXPECT_EQ(get_config.threshold1.enableHighToLow, set_config.threshold1.enableHighToLow);
        EXPECT_EQ(get_config.threshold1.threshold, set_config.threshold1.threshold);
        EXPECT_EQ(get_config.threshold2.enableLowToHigh, set_config.threshold2.enableLowToHigh);
        EXPECT_EQ(get_config.threshold2.enableHighToLow, set_config.threshold2.enableHighToLow);
        EXPECT_EQ(get_config.threshold2.threshold, set_config.threshold2.threshold);
        lzt::set_temp_configuration(tempHandle, initial_config);
      }
      }
    }
  }
TEST_F(TempModuleTest,
       GivenValidTempHandleWhenRetrievingTempStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto tempHandles = lzt::get_temp_handles(device, count);
    for (auto tempHandle : tempHandles) {
      ASSERT_NE(nullptr, tempHandle);
      uint32_t pCount = 0;
      auto freqHandles = lzt::get_freq_handles(device, pCount);
      for (auto freqHandle : freqHandles) {
        EXPECT_NE(nullptr, freqHandle);
        auto temp = lzt::get_temp_state(tempHandle);
        double tMax = 0;
        EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanFrequencyOcGetTjMax(freqHandle, &tMax));
        ASSERT_LE(temp, tMax);
        EXPECT_GT(temp, 0);
      }
    }
  }
}

} // namespace


