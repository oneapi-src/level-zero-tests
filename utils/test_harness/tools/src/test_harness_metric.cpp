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
#include <cstdlib>
#include <map>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_metric_group_count(ze_device_handle_t device) {
  uint32_t metricGroupCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGet(device, &metricGroupCount, nullptr));
  EXPECT_GT(metricGroupCount, 0);
  return metricGroupCount;
}
std::vector<zet_metric_group_handle_t>
get_metric_group_handles(ze_device_handle_t device) {
  auto metricGroupCount = get_metric_group_count(device);
  std::vector<zet_metric_group_handle_t> phMetricGroups(metricGroupCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricGroupGet(device, &metricGroupCount,
                                                 phMetricGroups.data()));
  return phMetricGroups;
}

uint32_t get_metric_count(zet_metric_group_handle_t metricGroup) {
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(metricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);
  return metricCount;
}
std::vector<zet_metric_handle_t>
get_metric_handles(zet_metric_group_handle_t metricGroup) {
  uint32_t metricCount = get_metric_count(metricGroup);
  std::vector<zet_metric_handle_t> phMetric(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(metricGroup, &metricCount, phMetric.data()));
  return phMetric;
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

bool check_metric_type_ip_exp(zet_metric_group_handle_t metricGroupHandle,
                              bool includeExpFeature) {

  std::vector<zet_metric_handle_t> metricHandles =
      get_metric_handles(metricGroupHandle);

  for (auto metric : metricHandles) {
    zet_metric_properties_t MetricProp = {};
    MetricProp.stype = ZET_STRUCTURE_TYPE_METRIC_PROPERTIES;
    MetricProp.pNext = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricGetProperties(metric, &MetricProp));

    if (MetricProp.metricType == ZET_METRIC_TYPE_IP_EXP && !includeExpFeature) {
      return true;
    }
  }
  return false;
}

bool check_metric_type_ip_exp(
    ze_device_handle_t device, std::string groupName,
    zet_metric_group_sampling_type_flags_t samplingType,
    bool includeExpFeature) {

  zet_metric_group_handle_t groupHandle;
  groupHandle = lzt::find_metric_group(device, groupName, samplingType);
  return check_metric_type_ip_exp(groupHandle, includeExpFeature);
}

std::vector<metricGroupInfo_t> optimize_metric_group_info_list(
    std::vector<metricGroupInfo_t> &metricGroupInfoList) {

  std::map<uint32_t, std::vector<metricGroupInfo_t>> domainMetricGroupMap = {};
  // Split the metric group info based on domains
  for (auto const &metricGroupInfo : metricGroupInfoList) {
    domainMetricGroupMap[metricGroupInfo.domain].push_back(metricGroupInfo);
  }

  std::vector<metricGroupInfo_t> optimizedList = {};
  optimizedList.reserve(metricGroupInfoList.size());
  // Consider 20% of the metric groups in each domain for test input
  uint32_t percentOfMetricGroupForTest = 20;
  const char *valueString = std::getenv("LZT_METRIC_GROUPS_TEST_PERCENTAGE");
  if (valueString != nullptr) {
    uint32_t value = atoi(valueString);
    percentOfMetricGroupForTest =
        value != 0 ? value : percentOfMetricGroupForTest;
    percentOfMetricGroupForTest = std::min(percentOfMetricGroupForTest, 100u);
  }

  for (auto const &mapEntry : domainMetricGroupMap) {
    uint32_t newCount = static_cast<uint32_t>(std::max<double>(
        mapEntry.second.size() * percentOfMetricGroupForTest * 0.01, 1.0));
    std::copy(mapEntry.second.begin(), mapEntry.second.begin() + newCount,
              std::back_inserter(optimizedList));
  }

  return optimizedList;
}

std::vector<metricGroupInfo_t>
get_metric_group_info(ze_device_handle_t device,
                      zet_metric_group_sampling_type_flags_t metricSamplingType,
                      bool includeExpFeature) {

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  std::vector<zet_metric_group_handle_t> metricGroupHandles =
      get_metric_group_handles(device);

  std::vector<metricGroupInfo_t> matchedGroupsInfo;

  for (auto metricGroupHandle : metricGroupHandles) {
    zet_metric_group_properties_t metricGroupProp = {};
    metricGroupProp.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    metricGroupProp.pNext = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGroupGetProperties(metricGroupHandle, &metricGroupProp));
    // samplingType is a bit mask.
    if (!(metricGroupProp.samplingType & metricSamplingType)) {
      continue;
    }

    if (check_metric_type_ip_exp(metricGroupHandle, includeExpFeature)) {
      LOG_WARNING
          << "Test includes ZET_METRIC_TYPE_IP_EXP Metric Type - Skipped...";
      continue;
    }

    matchedGroupsInfo.emplace_back(metricGroupHandle, metricGroupProp.name,
                                   metricGroupProp.domain);
  }

  return matchedGroupsInfo;
}

std::vector<std::string>
get_metric_group_name_list(ze_device_handle_t device,
                           zet_metric_group_sampling_type_flags_t samplingType,
                           bool includeExpFeature = true) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  auto groupProp = get_metric_group_properties(device);
  std::vector<std::string> groupPropName;
  for (auto propItem : groupProp) {
    std::string strItem(propItem.name);
    if (propItem.samplingType != samplingType)
      continue;
    if (check_metric_type_ip_exp(device, strItem, samplingType,
                                 includeExpFeature)) {
      LOG_WARNING
          << "Test includes ZET_METRIC_TYPE_IP_EXP Metric Type - Skipped...";
      continue;
    }
    groupPropName.push_back(strItem);
  }
  return groupPropName;
}

std::vector<zet_metric_group_properties_t> get_metric_group_properties(
    std::vector<zet_metric_group_handle_t> metricGroup) {
  std::vector<zet_metric_group_properties_t> metricGroupProp;
  for (auto mGroup : metricGroup) {
    zet_metric_group_properties_t GroupProp = {};
    GroupProp.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGroupGetProperties(mGroup, &GroupProp));
    metricGroupProp.push_back(GroupProp);
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

size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t reports) {
  size_t metricSize = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerReadData(metricStreamerHandle, reports,
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
            zetMetricStreamerReadData(metricStreamerHandle, UINT32_MAX,
                                      &metricSize, metricData->data()));
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

void verify_typed_metric_value(zet_typed_value_t result,
                               zet_value_type_t metricValueType) {
  EXPECT_EQ(metricValueType, result.type);
  switch (result.type) {
  case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
    EXPECT_GE(result.value.b8, std::numeric_limits<unsigned char>::min());
    EXPECT_LE(result.value.b8, std::numeric_limits<unsigned char>::max());
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
    EXPECT_GE(result.value.fp32, std::numeric_limits<float>::lowest());
    EXPECT_LE(result.value.fp32, std::numeric_limits<float>::max());
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
    EXPECT_GE(result.value.fp64, std::numeric_limits<double>::lowest());
    EXPECT_LE(result.value.fp64, std::numeric_limits<double>::max());
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
    EXPECT_GE(result.value.ui32, 0);
    EXPECT_LE(result.value.ui32, std::numeric_limits<uint32_t>::max());
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
    EXPECT_GE(result.value.ui64, 0);
    EXPECT_LE(result.value.ui64, std::numeric_limits<uint64_t>::max());
    break;
  default:
    ADD_FAILURE() << "Unexpected value type returned for metric query";
  }
}

void validate_metrics(zet_metric_group_handle_t hMetricGroup,
                      const size_t rawDataSize, const uint8_t *rawData) {

  // Get set count and total metric value count
  uint32_t setCount = 0;
  uint32_t totalMetricValueCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMultipleMetricValuesExp(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawDataSize, rawData, &setCount, &totalMetricValueCount,
                nullptr, nullptr));

  // Get metric counts and metric values
  std::vector<uint32_t> metricValueSets(setCount);
  std::vector<zet_typed_value_t> totalMetricValues(totalMetricValueCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMultipleMetricValuesExp(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawDataSize, rawData, &setCount, &totalMetricValueCount,
                metricValueSets.data(), totalMetricValues.data()));
  EXPECT_GT(totalMetricValueCount, 0);

  // Setup
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  LOG_DEBUG << "totalMetricValueCount " << totalMetricValueCount << " setCount "
            << setCount << " metricCount " << metricCount;
  // This loop over metric data is new for this extension
  uint32_t startIndex = 0;
  for (uint32_t dataIndex = 0; dataIndex < setCount; dataIndex++) {

    const uint32_t metricCountForDataIndex = metricValueSets[dataIndex];
    const uint32_t reportCount = metricCountForDataIndex / metricCount;

    for (uint32_t report = 0; report < reportCount; report++) {
      for (uint32_t metric = 0; metric < metricCount; metric++) {
        zet_metric_properties_t properties;
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zetMetricGetProperties(phMetrics[metric], &properties));
        const size_t metricIndex = report * metricCount + metric;
        zet_typed_value_t typed_value =
            totalMetricValues[startIndex + metricIndex];
        verify_typed_metric_value(typed_value, properties.resultType);
      }
    }

    startIndex += metricCountForDataIndex;
  }
  assert(startIndex == totalMetricValueCount);
}

} // namespace level_zero_tests
