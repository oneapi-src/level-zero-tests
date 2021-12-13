/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"
#include <cstring>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_metric_group_handles_count(ze_device_handle_t device) {
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGet(device, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);
  return metricCount;
}
std::vector<zet_metric_group_handle_t>
get_metric_group_handles(ze_device_handle_t device) {
  auto metricCount = get_metric_group_handles_count(device);
  std::vector<zet_metric_group_handle_t> metricGroup(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGet(device, &metricCount, metricGroup.data()));
  return metricGroup;
}

std::vector<ze_device_handle_t>
get_metric_test_device_list(uint32_t testSubDeviceCount) {
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t subDeviceCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
  subDeviceCount =
      testSubDeviceCount < subDeviceCount ? testSubDeviceCount : subDeviceCount;
  std::vector<ze_device_handle_t> testDevices(subDeviceCount + 1);
  if (subDeviceCount) {
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetSubDevices(device, &subDeviceCount,
                                                       testDevices.data()));
  }
  testDevices[subDeviceCount] = device;
  return testDevices;
}

std::vector<std::string> get_metric_group_name_list(
    ze_device_handle_t device,
    zet_metric_group_sampling_type_flags_t samplingType) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  auto groupProp = get_metric_group_properties(device);
  std::vector<std::string> groupPropName;
  for (auto propItem : groupProp) {
    std::string strItem(propItem.name);
    if (propItem.samplingType == samplingType)
      groupPropName.push_back(strItem);
  }
  return groupPropName;
}
std::vector<zet_metric_group_properties_t> get_metric_group_properties(
    std::vector<zet_metric_group_handle_t> metricGroup) {
  std::vector<zet_metric_group_properties_t> metricGroupProp;
  for (auto mGrpoup : metricGroup) {
    zet_metric_group_properties_t *GroupProp =
        new zet_metric_group_properties_t;
    std::memset(GroupProp, 0, sizeof(zet_metric_group_properties_t));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGroupGetProperties(mGrpoup, GroupProp));
    metricGroupProp.push_back(*GroupProp);
  }
  return metricGroupProp;
}
std::vector<zet_metric_group_properties_t>
get_metric_group_properties(ze_device_handle_t device) {
  std::vector<zet_metric_group_handle_t> metricGroupHandles =
      get_metric_group_handles(device);
  return get_metric_group_properties(metricGroupHandles);
}

zet_metric_group_handle_t find_metric_group(ze_device_handle_t device,
                                            std::string metricGroupToFind,
                                            uint32_t samplingType) {
  uint32_t i = 0;
  auto metricGroupHandles = lzt::get_metric_group_handles(device);
  auto metricGroupProp = get_metric_group_properties(device);
  for (auto groupProp : metricGroupProp) {
    if (strcmp(metricGroupToFind.c_str(), groupProp.name) == 0 &&
        (groupProp.samplingType == samplingType)) {
      break;
    }
    i++;
  }
  EXPECT_LE(i, metricGroupProp.size());
  if (i <= metricGroupProp.size()) {
    return metricGroupHandles.at(i);
  } else {
    return nullptr;
  }
}

zet_metric_query_pool_handle_t create_metric_query_pool_for_device(
    ze_device_handle_t device, zet_metric_query_pool_desc_t metricQueryPoolDesc,
    zet_metric_group_handle_t metricGroup) {
  zet_metric_query_pool_handle_t metric_query_pool;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricQueryPoolCreate(lzt::get_default_context(), device,
                                     metricGroup, &metricQueryPoolDesc,
                                     &metric_query_pool));
  EXPECT_NE(nullptr, metric_query_pool);
  return metric_query_pool;
}

zet_metric_query_pool_handle_t
create_metric_query_pool(uint32_t count, zet_metric_query_pool_type_t type,
                         zet_metric_group_handle_t metricGroup) {
  zet_metric_query_pool_desc_t metricQueryPoolDesc = {};
  metricQueryPoolDesc.count = count;
  metricQueryPoolDesc.type = type;

  metricQueryPoolDesc.pNext = nullptr;
  ze_device_handle_t device = zeDevice::get_instance()->get_device();
  return create_metric_query_pool_for_device(device, metricQueryPoolDesc,
                                             metricGroup);
}

zet_metric_query_pool_handle_t
create_metric_query_pool_for_device(ze_device_handle_t device, uint32_t count,
                                    zet_metric_query_pool_type_t type,
                                    zet_metric_group_handle_t metricGroup) {
  zet_metric_query_pool_desc_t metricQueryPoolDesc = {};
  metricQueryPoolDesc.count = count;
  metricQueryPoolDesc.type = type;

  metricQueryPoolDesc.pNext = nullptr;
  return create_metric_query_pool_for_device(device, metricQueryPoolDesc,
                                             metricGroup);
}

void destroy_metric_query_pool(
    zet_metric_query_pool_handle_t metric_query_pool_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricQueryPoolDestroy(metric_query_pool_handle));
}

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle) {
  zet_metric_query_handle_t metricQueryHandle;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricQueryCreate(metricQueryPoolHandle, 0, &metricQueryHandle));
  EXPECT_NE(nullptr, metricQueryHandle);

  return metricQueryHandle;
}

void destroy_metric_query(zet_metric_query_handle_t metricQueryHandle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricQueryDestroy(metricQueryHandle));
}

size_t metric_query_get_data_size(zet_metric_query_handle_t metricQueryHandle) {
  size_t metricSize = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricQueryGetData(metricQueryHandle, &metricSize, nullptr));
  return metricSize;
}
void metric_query_get_data(zet_metric_query_handle_t metricQueryHandle,
                           std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_query_get_data_size(metricQueryHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricQueryGetData(metricQueryHandle, &metricSize,
                                  metricData->data()));
}

void metric_streamer_close(zet_metric_streamer_handle_t metricStreamerHandle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricStreamerClose(metricStreamerHandle));
}
zet_metric_streamer_handle_t
metric_streamer_open(zet_metric_group_handle_t matchedGroupHandle,
                     ze_event_handle_t eventHandle,
                     uint32_t notifyEveryNReports, uint32_t samplingPeriod) {
  ze_device_handle_t device = zeDevice::get_instance()->get_device();
  zet_metric_streamer_handle_t metricStreamerHandle;
  zet_metric_streamer_desc_t metricStreamerDesc = {
      ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports,
      samplingPeriod};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerOpen(lzt::get_default_context(), device,
                                  matchedGroupHandle, &metricStreamerDesc,
                                  eventHandle, &metricStreamerHandle));
  EXPECT_NE(nullptr, metricStreamerHandle);
  return metricStreamerHandle;
}

zet_metric_streamer_handle_t metric_streamer_open_for_device(
    ze_device_handle_t device, zet_metric_group_handle_t matchedGroupHandle,
    ze_event_handle_t eventHandle, uint32_t notifyEveryNReports,
    uint32_t samplingPeriod) {
  zet_metric_streamer_handle_t metricStreamerHandle;
  zet_metric_streamer_desc_t metricStreamerDesc = {
      ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports,
      samplingPeriod};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerOpen(lzt::get_default_context(), device,
                                  matchedGroupHandle, &metricStreamerDesc,
                                  eventHandle, &metricStreamerHandle));
  EXPECT_NE(nullptr, metricStreamerHandle);
  return metricStreamerHandle;
}

void commandlist_append_streamer_marker(
    zet_command_list_handle_t commandList,
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t tracerMarker) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetCommandListAppendMetricStreamerMarker(
                commandList, metricStreamerHandle, tracerMarker));
}

size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle) {
  size_t metricSize = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerReadData(metricStreamerHandle, UINT32_MAX,
                                      &metricSize, nullptr));
  return metricSize;
}
void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle,
    std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_streamer_read_data_size(metricStreamerHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerReadData(metricStreamerHandle, 0, &metricSize,
                                      metricData->data()));
}

void activate_metric_groups(ze_device_handle_t device, uint32_t count,
                            zet_metric_group_handle_t matchedGroupHandle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetContextActivateMetricGroups(lzt::get_default_context(), device,
                                           count, &matchedGroupHandle));
}
void deactivate_metric_groups(ze_device_handle_t device) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetContextActivateMetricGroups(lzt::get_default_context(), device,
                                           0, nullptr));
}

void append_metric_query_begin(zet_command_list_handle_t commandList,
                               zet_metric_query_handle_t metricQueryHandle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetCommandListAppendMetricQueryBegin(
                                   commandList, metricQueryHandle));
}

void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetCommandListAppendMetricQueryEnd(commandList, metricQueryHandle,
                                               eventHandle, 0, nullptr));
}

void validate_metrics(zet_metric_group_handle_t matchedGroupHandle,
                      const size_t rawDataSize, const uint8_t *rawData) {
  uint32_t count = 0;
  std::vector<zet_typed_value_t> results;
  std::vector<zet_metric_handle_t> metrics;
  zet_metric_group_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMetricValues(
                matchedGroupHandle,
                ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES, rawDataSize,
                rawData, &count, nullptr));
  EXPECT_GT(count, 0);
  results.resize(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMetricValues(
                matchedGroupHandle,
                ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES, rawDataSize,
                rawData, &count, results.data()));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGetProperties(matchedGroupHandle, &properties));
  metrics.resize(properties.metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(matchedGroupHandle, &properties.metricCount,
                         metrics.data()));
  for (uint32_t i = 0; i < count; i++) {
    switch (results[i].type) {
    case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
      EXPECT_GE(results[i].value.b8, std::numeric_limits<unsigned char>::min());
      EXPECT_LE(results[i].value.b8, std::numeric_limits<unsigned char>::max());
      break;
    case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
      EXPECT_GE(results[i].value.fp32, std::numeric_limits<float>::lowest());
      EXPECT_LE(results[i].value.fp32, std::numeric_limits<float>::max());
      break;
    case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
      EXPECT_GE(results[i].value.fp64, std::numeric_limits<double>::lowest());
      EXPECT_LE(results[i].value.fp64, std::numeric_limits<double>::max());
      break;
    case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
      EXPECT_GE(results[i].value.ui32, 0);
      EXPECT_LE(results[i].value.ui32, std::numeric_limits<uint32_t>::max());
      break;
    case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
      EXPECT_GE(results[i].value.ui64, 0);
      EXPECT_LE(results[i].value.ui64, std::numeric_limits<uint64_t>::max());
      break;
    default:
      ADD_FAILURE() << "Unexpected value type returned for metric query";
    }
  }
  for (uint32_t i = 0; i < count; i++) {
    zet_metric_properties_t metricProperties = {};
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGetProperties(metrics[i % properties.metricCount],
                                     &metricProperties));

    if (metricProperties.metricType ==
        zet_metric_type_t::ZET_METRIC_TYPE_DURATION) {
      switch (results[i].type) {
      case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
        if (results[i].value.b8 > 0) {
          i = count;
          continue;
        }
        break;
      case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
        if (results[i].value.fp32 > 0) {
          i = count;
          continue;
        }
        break;
      case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
        if (results[i].value.fp64 > 0) {
          i = count;
          continue;
        }
        break;
      case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
        if (results[i].value.ui32 > 0) {
          i = count;
          continue;
        }
        break;
      case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
        if (results[i].value.ui64 > 0) {
          i = count;
          continue;
        }
        break;
      default:
        ADD_FAILURE() << "Unexpected value type returned for metric query";
      }
    }
    if (i == count - 1) {
      ADD_FAILURE() << "All duration metrics are zero";
    }
  }
}
} // namespace level_zero_tests
