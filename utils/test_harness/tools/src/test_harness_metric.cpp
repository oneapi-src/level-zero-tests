/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
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

void get_metric_properties(zet_metric_handle_t hMetric,
                           zet_metric_properties_t *metricProperties) {
  *metricProperties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES, nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGetProperties(hMetric, metricProperties));
}

std::string metric_type_to_string(zet_metric_type_t metric_type) {
  switch (metric_type) {
  case zet_metric_type_t::ZET_METRIC_TYPE_DURATION:
    return "ZET_METRIC_TYPE_DURATION";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_EVENT:
    return "ZET_METRIC_TYPE_EVENT";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_EVENT_WITH_RANGE:
    return "ZET_METRIC_TYPE_EVENT_WITH_RANGE";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_THROUGHPUT:
    return "ZET_METRIC_TYPE_EVENT_TYPE_THROUGHPUT";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_TIMESTAMP:
    return "ZET_METRIC_TYPE_EVENT_TYPE_TIMESTAMP";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_FLAG:
    return "ZET_METRIC_TYPE_EVENT_TYPE_FLAG";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_RATIO:
    return "ZET_METRIC_TYPE_EVENT_TYPE_RATIO";
    break;
  case zet_metric_type_t::ZET_METRIC_TYPE_RAW:
    return "ZET_METRIC_TYPE_EVENT_TYPE_RAW";
    break;
  default:
    LOG_WARNING << "invalid zet_metric_type_t encountered " << metric_type;
    return "UNKNOWN TYPE";
    break;
  }
}

std::string metric_value_type_to_string(zet_value_type_t result_type) {
  switch (result_type) {
  case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
    return "BOOL";
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
    return "fp32";
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
    return "fp64";
  case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
    return "uint32_t";
  case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
    return "uint64_t";
  default:
    return "UNKNOWN typed value";
  }
}

void logMetricGroupDebugInfo(const metricGroupInfo_t &groupInfo) {
  LOG_DEBUG << "#####################################";
  LOG_DEBUG << "test metricGroup name " << groupInfo.metricGroupName;
  LOG_DEBUG << "metric group description " << groupInfo.metricGroupDescription
            << " domain " << groupInfo.domain << " metricCount "
            << groupInfo.metricCount;

  std::vector<zet_metric_handle_t> metricHandles;
  metricHandles = lzt::get_metric_handles(groupInfo.metricGroupHandle);

  for (auto metricHandle : metricHandles) {
    zet_metric_properties_t metricProperties;
    lzt::get_metric_properties(metricHandle, &metricProperties);
    LOG_DEBUG << " ";
    LOG_DEBUG << "metric name " << metricProperties.name;
    LOG_DEBUG << " description " << metricProperties.description;
    LOG_DEBUG << " component " << metricProperties.component;
    LOG_DEBUG << " tierNumber " << metricProperties.tierNumber;
    LOG_DEBUG << " metricType "
              << metric_type_to_string(metricProperties.metricType);
    LOG_DEBUG << " resultType "
              << metric_value_type_to_string(metricProperties.resultType);
    LOG_DEBUG << " resultUnits " << metricProperties.resultUnits;
  }
  LOG_DEBUG << "#####################################";
};

std::vector<ze_device_handle_t> get_metric_test_no_subdevices_list(void) {
  std::vector<ze_device_handle_t> testDevices;
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();
  uint32_t subDeviceCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
  if (subDeviceCount) {
    testDevices.resize(subDeviceCount);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetSubDevices(device, &subDeviceCount,
                                                       testDevices.data()));
  } else {
    testDevices.resize(1);
    testDevices[0] = device;
  }
  return testDevices;
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

bool check_metric_type_ip(zet_metric_group_handle_t metricGroupHandle,
                          bool includeExpFeature) {

  std::vector<zet_metric_handle_t> metricHandles =
      get_metric_handles(metricGroupHandle);

  for (auto metric : metricHandles) {
    zet_metric_properties_t MetricProp = {};
    MetricProp.stype = ZET_STRUCTURE_TYPE_METRIC_PROPERTIES;
    MetricProp.pNext = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricGetProperties(metric, &MetricProp));

    if (MetricProp.metricType == ZET_METRIC_TYPE_IP && !includeExpFeature) {
      return true;
    }
  }
  return false;
}

bool check_metric_type_ip(ze_device_handle_t device, std::string groupName,
                          zet_metric_group_sampling_type_flags_t samplingType,
                          bool includeExpFeature) {

  zet_metric_group_handle_t groupHandle;
  groupHandle = lzt::find_metric_group(device, groupName, samplingType);
  return check_metric_type_ip(groupHandle, includeExpFeature);
}

std::vector<metricGroupInfo_t> optimize_metric_group_info_list(
    std::vector<metricGroupInfo_t> &metricGroupInfoList,
    uint32_t percentOfMetricGroupForTest, const char *metricGroupName) {

  std::vector<metricGroupInfo_t> optimizedList;

  const char *specificMetricGroupName = nullptr;

  const char *metricGroupNameEnvironmentVariable =
      std::getenv("LZT_METRIC_GROUPS_TEST_SPECIFIC");

  if (metricGroupNameEnvironmentVariable != nullptr) {
    specificMetricGroupName = metricGroupNameEnvironmentVariable;
  } else if (metricGroupName != nullptr) {
    specificMetricGroupName = metricGroupName;
  }
  if (specificMetricGroupName != nullptr) {
    LOG_INFO << "specific group " << specificMetricGroupName;
    for (auto const &metricGroupInfo : metricGroupInfoList) {
      if (metricGroupInfo.metricGroupName.compare(specificMetricGroupName) ==
          0) {
        LOG_INFO << "specific name push_back "
                 << metricGroupInfo.metricGroupName;
        optimizedList.push_back(metricGroupInfo);
        return optimizedList;
      }
    }
  }

  std::map<uint32_t, std::vector<metricGroupInfo_t>> domainMetricGroupMap;
  // Split the metric group info based on domains
  for (auto const &metricGroupInfo : metricGroupInfoList) {
    domainMetricGroupMap[metricGroupInfo.domain].push_back(metricGroupInfo);
  }

  optimizedList.reserve(metricGroupInfoList.size());

  // allow PERCENTAGE environment variable to override argument argument
  const char *valueString = std::getenv("LZT_METRIC_GROUPS_TEST_PERCENTAGE");
  if (valueString != nullptr) {
    uint32_t value = atoi(valueString);
    percentOfMetricGroupForTest =
        value != 0 ? value : percentOfMetricGroupForTest;
    percentOfMetricGroupForTest = std::min(percentOfMetricGroupForTest, 100u);
  }
  LOG_INFO << "percentage of metric groups " << percentOfMetricGroupForTest;

  for (auto const &mapEntry : domainMetricGroupMap) {
    uint32_t newCount = static_cast<uint32_t>(std::max<double>(
        mapEntry.second.size() * percentOfMetricGroupForTest * 0.01, 1.0));
    std::copy(mapEntry.second.begin(), mapEntry.second.begin() + newCount,
              std::back_inserter(optimizedList));
  }

  LOG_INFO << "size of optimizedList based on percentage "
           << optimizedList.size();
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

    if (check_metric_type_ip(metricGroupHandle, includeExpFeature)) {
      LOG_WARNING
          << "Test includes ZET_METRIC_TYPE_IP Metric Type - Skipped...";
      continue;
    }

    matchedGroupsInfo.emplace_back(
        metricGroupHandle, metricGroupProp.name, metricGroupProp.description,
        metricGroupProp.domain, metricGroupProp.metricCount);
  }

  return matchedGroupsInfo;
}

std::vector<metricGroupInfo_t> get_metric_type_ip_group_info(
    ze_device_handle_t device,
    zet_metric_group_sampling_type_flags_t metricSamplingType) {

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

    if (!check_metric_type_ip(metricGroupHandle, false)) {
      continue;
    }

    LOG_DEBUG << "Test INCLUDES Metric name ... " << metricGroupProp.name;
    matchedGroupsInfo.emplace_back(
        metricGroupHandle, metricGroupProp.name, metricGroupProp.description,
        metricGroupProp.domain, metricGroupProp.metricCount);
    continue;
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
    if (check_metric_type_ip(device, strItem, samplingType,
                             includeExpFeature)) {
      LOG_WARNING
          << "Test includes ZET_METRIC_TYPE_IP Metric Type - Skipped...";
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
    GroupProp.pNext = nullptr;
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
  zet_metric_query_pool_handle_t metric_query_pool = nullptr;

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
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle,
                    uint32_t index) {
  zet_metric_query_handle_t metricQueryHandle;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricQueryCreate(metricQueryPoolHandle,
                                                    index, &metricQueryHandle));
  EXPECT_NE(nullptr, metricQueryHandle);

  return metricQueryHandle;
}

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle) {
  zet_metric_query_handle_t metricQueryHandle;

  return metric_query_create(metricQueryPoolHandle, 0);
}

void destroy_metric_query(zet_metric_query_handle_t metricQueryHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricQueryDestroy(metricQueryHandle));
}

void reset_metric_query(zet_metric_query_handle_t &metricQueryHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricQueryReset(metricQueryHandle));
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
  zet_metric_streamer_handle_t metricStreamerHandle = nullptr;
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
    ze_event_handle_t eventHandle, uint32_t &notifyEveryNReports,
    uint32_t &samplingPeriod) {
  zet_metric_streamer_handle_t metricStreamerHandle = nullptr;
  zet_metric_streamer_desc_t metricStreamerDesc = {
      ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, notifyEveryNReports,
      samplingPeriod};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerOpen(lzt::get_default_context(), device,
                                  matchedGroupHandle, &metricStreamerDesc,
                                  eventHandle, &metricStreamerHandle));
  EXPECT_NE(nullptr, metricStreamerHandle);
  notifyEveryNReports = metricStreamerDesc.notifyEveryNReports;
  samplingPeriod = metricStreamerDesc.samplingPeriod;
  return metricStreamerHandle;
}

ze_result_t commandlist_append_streamer_marker(
    zet_command_list_handle_t commandList,
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t tracerMarker) {
  return zetCommandListAppendMetricStreamerMarker(
      commandList, metricStreamerHandle, tracerMarker);
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

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t &rawDataSize,
    std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_streamer_read_data_size(metricStreamerHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerReadData(metricStreamerHandle, UINT32_MAX,
                                      &metricSize, metricData->data()));
  rawDataSize = metricSize;
}

void activate_metric_groups(
    ze_device_handle_t device, uint32_t count,
    zet_metric_group_handle_t *ptr_matched_group_handle) {
  ASSERT_NE(nullptr, *ptr_matched_group_handle);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetContextActivateMetricGroups(lzt::get_default_context(), device,
                                           count, ptr_matched_group_handle));
}

void deactivate_metric_groups(ze_device_handle_t device) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetContextActivateMetricGroups(lzt::get_default_context(), device,
                                           0, nullptr));
}

void append_metric_query_begin(zet_command_list_handle_t commandList,
                               zet_metric_query_handle_t metricQueryHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetCommandListAppendMetricQueryBegin(
                                   commandList, metricQueryHandle));
}

void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  append_metric_query_end(commandList, metricQueryHandle, eventHandle, 0,
                          nullptr);
}

void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle,
                             uint32_t numWaitEvents,
                             ze_event_handle_t *waitEvents) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetCommandListAppendMetricQueryEnd(
                                   commandList, metricQueryHandle, eventHandle,
                                   numWaitEvents, waitEvents));
}

void append_metric_memory_barrier(zet_command_list_handle_t commandList) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetCommandListAppendMetricMemoryBarrier(commandList));
}

void verify_typed_metric_value(zet_typed_value_t result,
                               zet_value_type_t metricValueType) {
  EXPECT_EQ(metricValueType, result.type);
  switch (result.type) {
  case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
    EXPECT_GE(result.value.b8, std::numeric_limits<unsigned char>::min());
    EXPECT_LE(result.value.b8, std::numeric_limits<unsigned char>::max());
    LOG_DEBUG << "BOOL " << result.value.b8;
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
    EXPECT_GE(result.value.fp32, std::numeric_limits<float>::lowest());
    EXPECT_LE(result.value.fp32, std::numeric_limits<float>::max());
    LOG_DEBUG << "fp32 " << result.value.fp32;
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
    EXPECT_GE(result.value.fp64, std::numeric_limits<double>::lowest());
    EXPECT_LE(result.value.fp64, std::numeric_limits<double>::max());
    LOG_DEBUG << "fp64 " << result.value.fp64;
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
    EXPECT_GE(result.value.ui32, 0);
    EXPECT_LE(result.value.ui32, std::numeric_limits<uint32_t>::max());
    LOG_DEBUG << "uint32_t " << result.value.ui32;
    break;
  case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
    EXPECT_GE(result.value.ui64, 0);
    EXPECT_LE(result.value.ui64, std::numeric_limits<uint64_t>::max());
    LOG_DEBUG << "uint64_t " << result.value.ui64;
    break;
  default:
    ADD_FAILURE() << "Unexpected value type returned for metric query";
  }
}

void validate_metrics_common(
    zet_metric_group_handle_t hMetricGroup, const size_t rawDataSize,
    const uint8_t *rawData, bool requireOverflow, const char *metricNameForTest,
    std::vector<std::vector<zet_typed_value_t>> &typedValuesFound) {

  // Get set count and total metric value count
  uint32_t setCount = 0;
  uint32_t totalMetricValueCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMultipleMetricValuesExp(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawDataSize, rawData, &setCount, &totalMetricValueCount,
                nullptr, nullptr));

  LOG_DEBUG << "validate_metrics first calculate rawDataSize " << rawDataSize
            << " setCount " << setCount << " totalMetricValueCount "
            << totalMetricValueCount;

  // Get metric counts and metric values
  std::vector<uint32_t> metricValueSets(setCount);
  std::vector<zet_typed_value_t> totalMetricValues(totalMetricValueCount);
  ze_result_t result = zetMetricGroupCalculateMultipleMetricValuesExp(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawDataSize, rawData, &setCount, &totalMetricValueCount,
      metricValueSets.data(), totalMetricValues.data());

  LOG_DEBUG << "validate_metrics SECOND calculate rawDataSize " << rawDataSize
            << " setCount " << setCount << " totalMetricValueCount "
            << totalMetricValueCount << " result " << result;

  if (requireOverflow) {
    EXPECT_EQ(result, ZE_RESULT_WARNING_DROPPED_DATA);
  } else {
    EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
                result == ZE_RESULT_WARNING_DROPPED_DATA);
  }
  EXPECT_GT(totalMetricValueCount, 0);

  LOG_DEBUG << " rawDataSize " << rawDataSize << " setCount " << setCount
            << " totalMetricValueCount " << totalMetricValueCount;

  // Setup
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  LOG_INFO << "totalMetricValueCount " << totalMetricValueCount << " setCount "
           << setCount << " metricCount " << metricCount;
  // This loop over metric data is new for this extension
  uint32_t startIndex = 0;
  for (uint32_t dataIndex = 0; dataIndex < setCount; dataIndex++) {

    std::vector<zet_typed_value_t> valueVector;

    const uint32_t metricCountForDataIndex = metricValueSets[dataIndex];
    const uint32_t reportCount = metricCountForDataIndex / metricCount;

    LOG_DEBUG << "metricCountForDataIndex " << metricCountForDataIndex;
    LOG_DEBUG << "reportCount " << reportCount;

    for (uint32_t report = 0; report < reportCount; report++) {
      LOG_DEBUG << "BEGIN METRIC INSTANCE";
      for (uint32_t metric = 0; metric < metricCount; metric++) {
        zet_metric_properties_t properties = {
            ZET_STRUCTURE_TYPE_METRIC_PROPERTIES, nullptr};
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zetMetricGetProperties(phMetrics[metric], &properties));
        const size_t metricIndex = report * metricCount + metric;
        zet_typed_value_t typed_value =
            totalMetricValues[startIndex + metricIndex];
        verify_typed_metric_value(typed_value, properties.resultType);
        if ((metricNameForTest != nullptr)) {
          if (strncmp(metricNameForTest, properties.name,
                      strnlen(metricNameForTest, 1024)) == 0) {
            valueVector.push_back(typed_value);
          }
        }
      }
      LOG_DEBUG << "END METRIC INSTANCE";
    }

    LOG_DEBUG << "END METRIC SET";

    if ((metricNameForTest != nullptr)) {
      typedValuesFound.push_back(valueVector);
    }

    startIndex += metricCountForDataIndex;
  }
  assert(startIndex == totalMetricValueCount);
}

void validate_metrics(zet_metric_group_handle_t hMetricGroup,
                      const size_t rawDataSize, const uint8_t *rawData,
                      bool requireOverflow) {

  static std::vector<std::vector<zet_typed_value_t>> dummyDataVector;

  validate_metrics_common(hMetricGroup, rawDataSize, rawData, requireOverflow,
                          nullptr, dummyDataVector);
}

void validate_metrics(
    zet_metric_group_handle_t hMetricGroup, const size_t rawDataSize,
    const uint8_t *rawData, const char *metricNameForTest,
    std::vector<std::vector<zet_typed_value_t>> &typedValuesFoundPerDevice) {

  validate_metrics_common(hMetricGroup, rawDataSize, rawData, false,
                          metricNameForTest, typedValuesFoundPerDevice);
}

void validate_metrics_std(zet_metric_group_handle_t hMetricGroup,
                          const size_t rawDataSize, const uint8_t *rawData) {

  uint32_t metricValueCount = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMetricValues(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawDataSize, rawData, &metricValueCount, nullptr));

  // Get metric counts and metric values
  std::vector<zet_typed_value_t> metricValues(metricValueCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMetricValues(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawDataSize, rawData, &metricValueCount, metricValues.data()));
  EXPECT_GT(metricValueCount, 0);

  // Setup
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  LOG_DEBUG << "metricValueCount " << metricValueCount << " metricCount "
            << metricCount;

  for (uint32_t count = 0; count < metricValueCount; count++) {
    zet_metric_properties_t properties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
                                          nullptr};
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zetMetricGetProperties(phMetrics[count % metricCount], &properties));
    zet_typed_value_t typed_value = metricValues[count];
    verify_typed_metric_value(typed_value, properties.resultType);
  }
}

bool verify_value_type(zet_value_type_t value_type) {
  bool value_type_is_valid = true;
  switch (value_type) {
  case zet_value_type_t::ZET_VALUE_TYPE_UINT32:
  case zet_value_type_t::ZET_VALUE_TYPE_UINT64:
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT32:
  case zet_value_type_t::ZET_VALUE_TYPE_FLOAT64:
  case zet_value_type_t::ZET_VALUE_TYPE_BOOL8:
    break;
  default:
    value_type_is_valid = false;
    LOG_WARNING << "invalid zet_value_type_t encountered " << value_type;
    break;
  }
  return value_type_is_valid;
}

bool verify_metric_type(zet_metric_type_t metric_type) {
  bool metric_type_is_valid = true;
  switch (metric_type) {
  case zet_metric_type_t::ZET_METRIC_TYPE_DURATION:
  case zet_metric_type_t::ZET_METRIC_TYPE_EVENT:
  case zet_metric_type_t::ZET_METRIC_TYPE_EVENT_WITH_RANGE:
  case zet_metric_type_t::ZET_METRIC_TYPE_THROUGHPUT:
  case zet_metric_type_t::ZET_METRIC_TYPE_TIMESTAMP:
  case zet_metric_type_t::ZET_METRIC_TYPE_FLAG:
  case zet_metric_type_t::ZET_METRIC_TYPE_RATIO:
  case zet_metric_type_t::ZET_METRIC_TYPE_RAW:
  case zet_metric_type_t::ZET_METRIC_TYPE_IP:
    break;
  default:
    metric_type_is_valid = false;
    LOG_WARNING << "inalid zet_metric_type_t encountered " << metric_type;
    break;
  }
  return metric_type_is_valid;
}

bool validateMetricsStructures(zet_metric_group_handle_t hMetricGroup) {
  bool invalid_type_encountered = false;
  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  for (uint32_t metric = 0; metric < metricCount; metric++) {
    zet_metric_properties_t properties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
                                          nullptr};
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGetProperties(phMetrics[metric], &properties));
    invalid_type_encountered |= !verify_value_type(properties.resultType);
    invalid_type_encountered |= !verify_metric_type(properties.metricType);
  }

  return !invalid_type_encountered;
}

std::vector<zet_metric_group_handle_t>
get_metric_groups_with_different_domains(const ze_device_handle_t device,
                                         uint32_t metric_groups_per_domain) {
  std::vector<zet_metric_group_handle_t> metric_group_handles =
      lzt::get_metric_group_handles(device);

  zet_metric_group_properties_t metric_group_property = {};
  metric_group_property.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  metric_group_property.pNext = nullptr;

  std::vector<uint32_t> unique_domains = {};
  std::vector<zet_metric_group_handle_t> unique_domain_metric_group_list = {};

  // Identify unique domains
  for (auto metric_group_handle : metric_group_handles) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGroupGetProperties(metric_group_handle,
                                          &metric_group_property));
    if (std::find(unique_domains.begin(), unique_domains.end(),
                  metric_group_property.domain) == unique_domains.end()) {
      unique_domains.push_back(metric_group_property.domain);
    }
  }

  // Collect metric groups for each domain
  for (auto &domain : unique_domains) {
    uint32_t metric_group_count = 0;
    for (auto metric_group_handle : metric_group_handles) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zetMetricGroupGetProperties(metric_group_handle,
                                            &metric_group_property));
      if (metric_group_property.domain == domain) {
        unique_domain_metric_group_list.push_back(metric_group_handle);
        ++metric_group_count;
      }

      if (metric_group_count >= metric_groups_per_domain) {
        break;
      }
    }
  }
  return unique_domain_metric_group_list;
}

void metric_calculate_metric_values_from_raw_data(
    zet_metric_group_handle_t hMetricGroup, std::vector<uint8_t> &rawData,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets, bool expect_overflow) {

  uint32_t setCount = 0;
  uint32_t totalMetricValueCount = 0;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupCalculateMultipleMetricValuesExp(
                hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
                rawData.size(), rawData.data(), &setCount,
                &totalMetricValueCount, nullptr, nullptr));

  // Get metric counts and metric values
  metricValueSets.resize(setCount);
  totalMetricValues.resize(totalMetricValueCount);

  ze_result_t result = zetMetricGroupCalculateMultipleMetricValuesExp(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawData.size(), rawData.data(), &setCount, &totalMetricValueCount,
      metricValueSets.data(), totalMetricValues.data());

  LOG_INFO << "metric_calculate_metric_values_from_raw_data: "
              "rawDataSize "
           << rawData.size() << " setCount " << setCount
           << " totalMetricValueCount " << totalMetricValueCount << " result "
           << result;

  if (expect_overflow) {
    EXPECT_TRUE(result == ZE_RESULT_WARNING_DROPPED_DATA);
  } else {
    EXPECT_TRUE(result == ZE_RESULT_SUCCESS);
  }
  EXPECT_GT(totalMetricValueCount, 0);
}

void metric_get_metric_handles_from_metric_group(
    zet_metric_group_handle_t hMetricGroup,
    std::vector<zet_metric_handle_t> &hMetrics) {

  uint32_t metricCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  hMetrics.resize(metricCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGet(hMetricGroup, &metricCount, hMetrics.data()));

  LOG_INFO << "number of metrics in metric group: metricCount " << metricCount;
}

void metric_get_metric_properties_for_metric_group(
    std::vector<zet_metric_handle_t> &metricHandles,
    std::vector<zet_metric_properties_t> &metricProperties) {

  for (uint32_t metric = 0; metric < metricHandles.size(); metric++) {
    metricProperties[metric] = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES, nullptr};
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetMetricGetProperties(metricHandles[metric],
                                     &metricProperties[metric]));
  }
}

void metric_validate_streamer_marker_data(
    std::vector<zet_metric_properties_t> &metricProperties,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets,
    std::vector<uint32_t> &streamerMarkerValues) {

  uint32_t numbefOfStreamerMarkerMatches = 0;
  uint32_t startIndex = 0;
  for (uint32_t dataIndex = 0; dataIndex < metricValueSets.size();
       dataIndex++) {

    const uint32_t metricCountForDataIndex = metricValueSets[dataIndex];
    const uint32_t reportCount =
        metricCountForDataIndex / metricProperties.size();
    uint32_t streamerMarkerValuesIndex = 0;

    LOG_INFO << "for dataIndex " << dataIndex << " metricCountForDataIndex "
             << metricCountForDataIndex << " reportCount " << reportCount;

    for (uint32_t report = 0; report < reportCount; report++) {

      for (uint32_t metric = 0; metric < metricProperties.size(); metric++) {

        const size_t metricIndex = report * metricProperties.size() + metric;
        zet_metric_properties_t metricProperty = metricProperties[metric];
        zet_typed_value_t metricTypedValue =
            totalMetricValues[startIndex + metricIndex];

        if ((strcmp("StreamMarker", metricProperty.name) == 0) &&
            (metricTypedValue.value.ui64 != 0)) {
          LOG_INFO << "Valid Streamer Marker found with value: "
                   << metricTypedValue.value.ui64;
          EXPECT_EQ(metricTypedValue.value.ui64,
                    streamerMarkerValues[streamerMarkerValuesIndex]);
          streamerMarkerValuesIndex++;
        }
      }
    }

    EXPECT_EQ(streamerMarkerValuesIndex, streamerMarkerValues.size());
    if (streamerMarkerValuesIndex == streamerMarkerValues.size()) {
      numbefOfStreamerMarkerMatches++;
    }

    startIndex += metricCountForDataIndex;
  }

  EXPECT_EQ(numbefOfStreamerMarkerMatches, metricValueSets.size());
}

} // namespace level_zero_tests
