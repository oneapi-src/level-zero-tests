/*
 *
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_metric_utils.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

class zetMetricMetricProgrammableTest : public ::testing::Test {
protected:
  std::vector<lzt::metric_programmable_handle_list_for_device>
      metric_programmable_lists_with_devices;

  void SetUp() override {
    driver = lzt::get_default_driver();
    device = lzt::get_default_device(driver);
    context = lzt::create_context(driver);
    devices = lzt::get_metric_test_device_list();

    initialize_limits_from_env_variables();

    lzt::generate_device_list_with_metric_programmable_handles(
        devices, metric_programmable_lists_with_devices,
        metric_programmable_handles_limit);
  }

  void TearDown() override { lzt::destroy_context(context); }

  void initialize_limits_from_env_variables() {

    const char *metric_group_handles_limit_value_string =
        std::getenv("LZT_METRIC_GROUP_HANDLES_LIMIT");
    if (metric_group_handles_limit_value_string != nullptr) {
      LOG_DEBUG << "metric_group_handles_limit_value_string "
                << metric_group_handles_limit_value_string;
      uint32_t value = atoi(metric_group_handles_limit_value_string);
      metric_group_handles_limit = value;
    }

    const char *metric_handles_limit_value_string =
        std::getenv("LZT_METRIC_HANDLES_LIMIT");
    if (metric_handles_limit_value_string != nullptr) {
      LOG_DEBUG << "metric_handles_limit_value_string "
                << metric_handles_limit_value_string;
      uint32_t value = atoi(metric_handles_limit_value_string);
      metric_handles_limit = value;
    }

    const char *info_limit_value_string =
        std::getenv("LZT_METRIC_PROGRAMMABLE_PARAM_INFO_LIMIT");
    if (info_limit_value_string != nullptr) {
      LOG_DEBUG << "info_limit_value_string " << info_limit_value_string;
      uint32_t value = atoi(info_limit_value_string);
      metric_programmable_param_info_limit = value;
    }

    const char *metric_programmable_limit_value_string =
        std::getenv("LZT_METRIC_PROGRAMMABLE_LIMIT");
    if (metric_programmable_limit_value_string != nullptr) {
      LOG_DEBUG << "metric_programmable_limit_value_string "
                << metric_programmable_limit_value_string;
      uint32_t value = atoi(metric_programmable_limit_value_string);
      metric_programmable_handles_limit = value;
    }

    LOG_DEBUG << "initialize from environment variable "
              << "LZT_METRIC_HANDLES_LIMIT :" << metric_handles_limit;

    LOG_DEBUG << "initialize from environment Variable: "
                 "LZT_METRIC_PROGRAMMABLE_PARAM_INFO_LIMIT : "
              << metric_programmable_param_info_limit;

    LOG_DEBUG << "initialize from environment Variable "
              << "LZT_METRIC_PROGRAMMABLE_LIMIT : "
              << metric_programmable_handles_limit;

    LOG_DEBUG << "initialize from environment Variable "
              << "LZT_METRIC_GROUP_HANDLES_LIMIT : "
              << metric_programmable_handles_limit;
  }

  ze_device_handle_t device;
  ze_driver_handle_t driver;
  ze_context_handle_t context;

  std::vector<ze_device_handle_t> devices;

  uint32_t metric_handles_limit = 10;
  uint32_t metric_programmable_param_info_limit = 10;
  uint32_t metric_programmable_handles_limit = 10;
  uint32_t metric_group_handles_limit = 10;

  void generate_metric_groups_from_metrics(
      ze_device_handle_t device_handle,
      std::vector<zet_metric_handle_t> &metric_handles,
      const std::string metric_group_name_prefix,
      const std::string metric_group_description,
      std::vector<zet_metric_group_handle_t> &metric_group_handles,
      uint32_t metric_group_handles_limit) {
    LOG_DEBUG << "ENTER generate_metric_groups_from_metrics "
                 "metric_group_handles_limit "
              << metric_group_handles_limit;

    ze_result_t result;
    uint32_t metric_group_handles_count = 0;

    result = zetDeviceCreateMetricGroupsFromMetricsExp(
        device_handle, metric_handles.size(), metric_handles.data(),
        metric_group_name_prefix.c_str(), metric_group_description.c_str(),
        &metric_group_handles_count, nullptr);

    ASSERT_EQ(result, ZE_RESULT_SUCCESS)
        << "creation of metric groups with zero metric count has failed";
    ASSERT_GT(metric_group_handles_count, 0)
        << "at least one metric group should have been created, but non were "
           "created.";

    LOG_INFO << "number of metric group handles found: "
             << metric_group_handles_count;

    uint32_t metric_group_handles_subset_size = metric_group_handles_count;
    if (metric_group_handles_limit) {
      metric_group_handles_subset_size = std::min(
          metric_group_handles_subset_size, metric_group_handles_count);
      LOG_INFO << "metric group handles count reduced to "
               << metric_group_handles_subset_size;
    }

    metric_group_handles.resize(metric_group_handles_subset_size);
    result = zetDeviceCreateMetricGroupsFromMetricsExp(
        device_handle, metric_handles.size(), metric_handles.data(),
        metric_group_name_prefix.c_str(), metric_group_description.c_str(),
        &metric_group_handles_subset_size, metric_group_handles.data());
    ASSERT_EQ(result, ZE_RESULT_SUCCESS)
        << "creation of metric groups with non-zero metric count has failed";

    LOG_DEBUG << "LEAVE generate_metric_groups_from_metrics";
  }

  void destroy_metric_group_handles_list(
      std::vector<zet_metric_group_handle_t> metric_group_handles_list) {
    LOG_DEBUG << "ENTER destroy_metric_group_handles_list of size "
              << metric_group_handles_list.size();
    for (auto metric_group_handle : metric_group_handles_list) {
      ze_result_t result;

      result = zetMetricGroupDestroyExp(metric_group_handle);
      EXPECT_EQ(result, ZE_RESULT_SUCCESS)
          << "metric group destroy on handle has failed";
    }
    LOG_DEBUG << "LEAVE destroy_metric_group_handles_list";
  }
};

TEST_F(
    zetMetricMetricProgrammableTest,
    GivenValidDeviceSupportingMetricProgrammableThenMetricProgrammableGetYieldsAtLeastOneMetricProgrammableHandle) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
  }
}

TEST_F(zetMetricMetricProgrammableTest,
       GivenMetricProgrammableGetThenMetricProgrammablePropertiesAreValid) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
    ASSERT_GT(device_with_metric_programmable_groups
                  .metric_programmable_handles_for_device.size(),
              0u);
    for (auto &metric_programmable_handle :
         device_with_metric_programmable_groups
             .metric_programmable_handles_for_device) {
      ze_result_t result;
      zet_metric_programmable_exp_properties_t metric_programmable_properties{
          ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_EXP_PROPERTIES, nullptr};
      result = zetMetricProgrammableGetPropertiesExp(
          metric_programmable_handle, &metric_programmable_properties);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricProgrammableGetPropertiesExp failed retrieving metric "
             "properties for a metric programmable handle with error code"
          << result;

      size_t name_len;
      name_len = strnlen(metric_programmable_properties.name,
                         ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP);
      EXPECT_GT(name_len, 0)
          << "metric programmable property 'name' has an invalid zero length";
      EXPECT_LT(name_len, ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP)
          << "metric programmable property 'name' has an invalid length that "
             "is greater than "
          << ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP;

      size_t description_len;
      description_len = strnlen(metric_programmable_properties.description,
                                ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP);
      EXPECT_GT(description_len, 0)
          << "metric programmable property 'description' string has an invalid "
             "zero length";
      EXPECT_LT(description_len, ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP)
          << "metric programmable property 'description' string has an invalid "
             "length of "
          << description_len << ". Valid string lengths are less then "
          << ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP;

      size_t component_len;
      component_len = strnlen(metric_programmable_properties.component,
                              ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP);
      EXPECT_GT(component_len, 0) << "metric programmable property 'component' "
                                     "string has an invalid zero length";
      EXPECT_LT(component_len, ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP)
          << "metric programmable property 'component' string has an invalid "
             "length of "
          << component_len << ". Valid string lengths are less then "
          << ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP;

      constexpr uint32_t invalid_sampling_types =
          ~(ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED |
            ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED |
            ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED);

      uint32_t test_invalid_sampling_types =
          metric_programmable_properties.samplingType & invalid_sampling_types;
      EXPECT_EQ(test_invalid_sampling_types, 0)
          << "metric programmable properties contains one or more invalid "
             "sampling types: "
          << test_invalid_sampling_types;

      std::string sampling_type_names;
      if (metric_programmable_properties.samplingType &
          ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED) {
        sampling_type_names.append(
            "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED ");
      }
      if (metric_programmable_properties.samplingType &
          ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED) {
        sampling_type_names.append(
            "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED ");
      }
      if (metric_programmable_properties.samplingType &
          ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED) {
        sampling_type_names.append(
            "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED ");
      }

      EXPECT_NE(metric_programmable_properties.parameterCount, 0)
          << "metric programmable property parameter Count should not have a "
             "zero value";

      LOG_DEBUG << "metric programmable properties: 'name:' "
                << metric_programmable_properties.name << ", 'description': "
                << metric_programmable_properties.description
                << ", 'Component': " << metric_programmable_properties.component
                << " 'sampling types': " << sampling_type_names;
      LOG_DEBUG << " 'tier number': "
                << metric_programmable_properties.tierNumber << " domain "
                << metric_programmable_properties.domain
                << " 'parameter count': "
                << metric_programmable_properties.parameterCount
                << " 'source Id': " << metric_programmable_properties.sourceId;
    }
  }
}

TEST_F(
    zetMetricMetricProgrammableTest,
    GivenMetricProgrammablePropertiesThenGetParamInfoExpReturnsValidParamInfoExp) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
    ASSERT_GT(device_with_metric_programmable_groups
                  .metric_programmable_handles_for_device.size(),
              0u);
    for (auto &metric_programmable_handle :
         device_with_metric_programmable_groups
             .metric_programmable_handles_for_device) {

      std::vector<zet_metric_programmable_param_info_exp_t> param_infos;

      lzt::generate_param_info_exp_list_from_metric_programmable(
          metric_programmable_handle, param_infos,
          metric_programmable_param_info_limit);
      ASSERT_GT(param_infos.size(), 0u);
      for (auto param_info : param_infos) {
        bool success;
        std::string type_string;
        std::string value_info_type_string;
        std::string value_info_type_default_value_string;

        success = lzt::programmable_param_info_validate(
            param_info, type_string, value_info_type_string,
            value_info_type_default_value_string);

        LOG_INFO << "parameter info type " << type_string
                 << " value_info_type_string " << value_info_type_string
                 << " value_info_type_default_value_string "
                 << value_info_type_default_value_string;
        EXPECT_TRUE(success)
            << "An invalid param_info_exp_t structure was encountered";
      }
    }
  }
}

TEST_F(
    zetMetricMetricProgrammableTest,
    GivenMetricProgrammableParamInfoExpThenGetParamValueInfoExpAndParamValueInfoDescExpReturnValidParamInfoExp) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
    ASSERT_GT(device_with_metric_programmable_groups
                  .metric_programmable_handles_for_device.size(),
              0u);
    for (auto &metric_programmable_handle :
         device_with_metric_programmable_groups
             .metric_programmable_handles_for_device) {

      std::vector<zet_metric_programmable_param_info_exp_t> param_infos;

      lzt::generate_param_info_exp_list_from_metric_programmable(
          metric_programmable_handle, param_infos,
          metric_programmable_param_info_limit);

      uint32_t param_info_ordinal = 0;
      ASSERT_GT(param_infos.size(), 0u);
      for (auto param_info : param_infos) {
        bool success;
        std::string type_string;
        std::string value_info_type_string;
        std::string value_info_type_default_value_string;

        success = lzt::programmable_param_info_validate(
            param_info, type_string, value_info_type_string,
            value_info_type_default_value_string);

        LOG_DEBUG << "parameter info type " << type_string
                  << " value_info_type_string " << value_info_type_string
                  << " value_info_type_default_value_string "
                  << value_info_type_default_value_string;
        EXPECT_TRUE(success)
            << "An invalid param_info_exp_t structure was encountered";
        LOG_INFO << "test param_value_exp_t only";
        lzt::generate_param_value_info_list_from_param_info(
            metric_programmable_handle, param_info_ordinal,
            param_info.valueInfoCount, param_info.valueInfoType, false);
        LOG_INFO << "test param_value_exp_t AND param_value_info_desc_exp_t";
        lzt::generate_param_value_info_list_from_param_info(
            metric_programmable_handle, param_info_ordinal,
            param_info.valueInfoCount, param_info.valueInfoType, true);
        param_info_ordinal++;
      }
    }
  }
}

TEST_F(
    zetMetricMetricProgrammableTest,
    GivenOneOrMoreMetricProgrammableHandlesThenCreateAndDestroyMetricsWillSucceed) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
    ASSERT_GT(device_with_metric_programmable_groups
                  .metric_programmable_handles_for_device.size(),
              0u);
    for (auto &metric_programmable_handle :
         device_with_metric_programmable_groups
             .metric_programmable_handles_for_device) {

      std::vector<zet_metric_programmable_param_info_exp_t> param_infos;

      lzt::generate_param_info_exp_list_from_metric_programmable(
          metric_programmable_handle, param_infos,
          metric_programmable_param_info_limit);

      zet_metric_programmable_exp_properties_t metric_programmable_properties;
      lzt::fetch_metric_programmable_exp_properties(
          metric_programmable_handle, metric_programmable_properties);

      std::vector<zet_metric_programmable_param_value_exp_t> parameter_values(
          param_infos.size());

      uint32_t param_info_index = 0;
      ASSERT_GT(param_infos.size(), 0u);
      for (auto param_info : param_infos) {
        parameter_values[param_info_index].value = param_info.defaultValue;
        param_info_index++;
      }

      std::vector<zet_metric_handle_t> metric_handles;

      lzt::generate_metric_handles_list_from_param_values(
          metric_programmable_handle, metric_programmable_properties.name,
          metric_programmable_properties.description, parameter_values,
          metric_handles, metric_handles_limit);

      lzt::destroy_metric_handles_list(metric_handles);
    }
  }
}

TEST_F(
    zetMetricMetricProgrammableTest,
    GivenOneOrMoreMetricProgrammableThenCreateAndDestroyMetricGroupSuccessfully) {

  if (metric_programmable_lists_with_devices.size() == 0) {
    GTEST_SKIP()
        << "There are no devices found that support metric programmable";
  }

  for (auto device_with_metric_programmable_groups :
       metric_programmable_lists_with_devices) {

    LOG_INFO << "description of the device being tested: "
             << device_with_metric_programmable_groups.device_description;
    LOG_INFO << "this device supports: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handle_count
             << " metric programmables but we are testing only a subset of: "
             << device_with_metric_programmable_groups
                    .metric_programmable_handles_for_device.size();
    ASSERT_GT(device_with_metric_programmable_groups
                  .metric_programmable_handles_for_device.size(),
              0u);
    for (auto &metric_programmable_handle :
         device_with_metric_programmable_groups
             .metric_programmable_handles_for_device) {

      std::vector<zet_metric_programmable_param_info_exp_t> param_infos;

      lzt::generate_param_info_exp_list_from_metric_programmable(
          metric_programmable_handle, param_infos,
          metric_programmable_param_info_limit);

      zet_metric_programmable_exp_properties_t metric_programmable_properties;
      lzt::fetch_metric_programmable_exp_properties(
          metric_programmable_handle, metric_programmable_properties);

      std::vector<zet_metric_programmable_param_value_exp_t> parameter_values(
          param_infos.size());

      uint32_t param_info_index = 0;
      ASSERT_GT(param_infos.size(), 0u);
      for (auto param_info : param_infos) {
        parameter_values[param_info_index].value = param_info.defaultValue;
        param_info_index++;
      }

      std::vector<zet_metric_handle_t> metric_handles;

      lzt::generate_metric_handles_list_from_param_values(
          metric_programmable_handle, metric_programmable_properties.name,
          metric_programmable_properties.description, parameter_values,
          metric_handles, metric_handles_limit);

      if (metric_handles.size() == 0) {
        LOG_WARNING << "we have encountered a programmable metric with no "
                       "metric_handles";
        continue;
      }

      std::vector<zet_metric_group_handle_t> metric_group_handles;
      generate_metric_groups_from_metrics(
          device, metric_handles, "group_name_prefix", "group_description",
          metric_group_handles, metric_group_handles_limit);

      destroy_metric_group_handles_list(metric_group_handles);

      lzt::destroy_metric_handles_list(metric_handles);
    }
  }
}

} // namespace
