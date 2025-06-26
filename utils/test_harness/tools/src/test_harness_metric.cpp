/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <map>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_metric_group_count(ze_device_handle_t device) {
  uint32_t metricGroupCount = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGroupGet(device, &metricGroupCount, nullptr));
  EXPECT_GT(metricGroupCount, 0);
  return metricGroupCount;
}
std::vector<zet_metric_group_handle_t>
get_metric_group_handles(ze_device_handle_t device) {
  auto metricGroupCount = get_metric_group_count(device);
  std::vector<zet_metric_group_handle_t> phMetricGroups(metricGroupCount);
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGroupGet(device, &metricGroupCount, phMetricGroups.data()));
  return phMetricGroups;
}

uint32_t get_metric_count(zet_metric_group_handle_t metricGroup) {
  uint32_t metricCount = 0;
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGet(metricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);
  return metricCount;
}
std::vector<zet_metric_handle_t>
get_metric_handles(zet_metric_group_handle_t metricGroup) {
  uint32_t metricCount = get_metric_count(metricGroup);
  std::vector<zet_metric_handle_t> phMetric(metricCount);
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGet(metricGroup, &metricCount, phMetric.data()));
  return phMetric;
}

void get_metric_properties(zet_metric_handle_t hMetric,
                           zet_metric_properties_t *metricProperties) {
  *metricProperties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES, nullptr};
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGetProperties(hMetric, metricProperties));
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
  EXPECT_ZE_RESULT_SUCCESS(
      zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
  if (subDeviceCount) {
    testDevices.resize(subDeviceCount);
    EXPECT_ZE_RESULT_SUCCESS(
        zeDeviceGetSubDevices(device, &subDeviceCount, testDevices.data()));
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
  EXPECT_ZE_RESULT_SUCCESS(
      zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
  subDeviceCount =
      testSubDeviceCount < subDeviceCount ? testSubDeviceCount : subDeviceCount;
  std::vector<ze_device_handle_t> testDevices(subDeviceCount + 1);
  if (subDeviceCount) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeDeviceGetSubDevices(device, &subDeviceCount, testDevices.data()));
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
    EXPECT_ZE_RESULT_SUCCESS(zetMetricGetProperties(metric, &MetricProp));

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

std::vector<zet_metric_group_handle_t> get_one_metric_group_per_domain(
    ze_device_handle_t device,
    std::vector<zet_metric_group_handle_t> &metricGroupHandleList) {
  std::vector<zet_metric_group_handle_t> concurrentMetricGroupList{};
  if (metricGroupHandleList.size() != 0) {
    uint32_t concurrentGroupCount = 0;
    EXPECT_ZE_RESULT_SUCCESS(zetDeviceGetConcurrentMetricGroupsExp(
        device, metricGroupHandleList.size(), metricGroupHandleList.data(),
        nullptr, &concurrentGroupCount));
    std::vector<uint32_t> countPerConcurrentGroup(concurrentGroupCount);
    EXPECT_ZE_RESULT_SUCCESS(zetDeviceGetConcurrentMetricGroupsExp(
        device, metricGroupHandleList.size(), metricGroupHandleList.data(),
        countPerConcurrentGroup.data(), &concurrentGroupCount));

    uint32_t metricGroupCountInConcurrentGroup = countPerConcurrentGroup[0];

    for (uint32_t i = 0; i < metricGroupCountInConcurrentGroup; i++) {
      concurrentMetricGroupList.push_back(metricGroupHandleList[i]);
    }
  }
  return concurrentMetricGroupList;
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
                      bool includeExpFeature, bool one_group_per_domain) {

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  std::vector<zet_metric_group_handle_t> metricGroupHandles =
      get_metric_group_handles(device);

  std::vector<metricGroupInfo_t> matchedGroupsInfo;
  std::vector<zet_metric_group_handle_t> concurrentMetricGroupHandles;

  for (auto metricGroupHandle : metricGroupHandles) {
    zet_metric_group_properties_t metricGroupProp = {};
    metricGroupProp.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    metricGroupProp.pNext = nullptr;
    EXPECT_ZE_RESULT_SUCCESS(
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
    if (one_group_per_domain) {
      concurrentMetricGroupHandles.push_back(metricGroupHandle);
    }
    matchedGroupsInfo.emplace_back(
        metricGroupHandle, metricGroupProp.name, metricGroupProp.description,
        metricGroupProp.domain, metricGroupProp.metricCount);
  }
  if (one_group_per_domain) {
    concurrentMetricGroupHandles =
        get_one_metric_group_per_domain(device, concurrentMetricGroupHandles);
    std::vector<metricGroupInfo_t> concurrentMatchedGroupsInfo;

    for (auto groupsInfo : matchedGroupsInfo) {
      if (count(concurrentMetricGroupHandles.begin(),
                concurrentMetricGroupHandles.end(),
                groupsInfo.metricGroupHandle)) {
        concurrentMatchedGroupsInfo.push_back(groupsInfo);
      }
    }

    return concurrentMatchedGroupsInfo;
  } else {
    return matchedGroupsInfo;
  }
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
    EXPECT_ZE_RESULT_SUCCESS(
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
    EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupGetProperties(mGroup, &GroupProp));
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

  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricQueryPoolCreate(lzt::get_default_context(), device, metricGroup,
                               &metricQueryPoolDesc, &metric_query_pool));
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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricQueryPoolDestroy(metric_query_pool_handle));
}

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle,
                    uint32_t index) {
  zet_metric_query_handle_t metricQueryHandle;
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricQueryCreate(metricQueryPoolHandle, index, &metricQueryHandle));
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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricQueryDestroy(metricQueryHandle));
}

void reset_metric_query(zet_metric_query_handle_t &metricQueryHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricQueryReset(metricQueryHandle));
}

size_t metric_query_get_data_size(zet_metric_query_handle_t metricQueryHandle) {
  size_t metricSize = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricQueryGetData(metricQueryHandle, &metricSize, nullptr));
  return metricSize;
}
void metric_query_get_data(zet_metric_query_handle_t metricQueryHandle,
                           std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_query_get_data_size(metricQueryHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricQueryGetData(metricQueryHandle, &metricSize,
                                                 metricData->data()));
}

void metric_streamer_close(zet_metric_streamer_handle_t metricStreamerHandle) {
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerClose(metricStreamerHandle));
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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerOpen(
      lzt::get_default_context(), device, matchedGroupHandle,
      &metricStreamerDesc, eventHandle, &metricStreamerHandle));
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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerOpen(
      lzt::get_default_context(), device, matchedGroupHandle,
      &metricStreamerDesc, eventHandle, &metricStreamerHandle));
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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerReadData(
      metricStreamerHandle, UINT32_MAX, &metricSize, nullptr));
  return metricSize;
}

size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t reports) {
  size_t metricSize = 0;
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerReadData(
      metricStreamerHandle, reports, &metricSize, nullptr));
  return metricSize;
}

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle,
    std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_streamer_read_data_size(metricStreamerHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerReadData(
      metricStreamerHandle, UINT32_MAX, &metricSize, metricData->data()));
}

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t &rawDataSize,
    std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  size_t metricSize = metric_streamer_read_data_size(metricStreamerHandle);
  EXPECT_GT(metricSize, 0);
  metricData->resize(metricSize);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerReadData(
      metricStreamerHandle, UINT32_MAX, &metricSize, metricData->data()));
  rawDataSize = metricSize;
}

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t reports,
    size_t &rawDataSize, std::vector<uint8_t> *metricData) {
  ASSERT_NE(nullptr, metricData);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricStreamerReadData(
      metricStreamerHandle, reports, &rawDataSize, metricData->data()));
}

void activate_metric_groups(
    ze_device_handle_t device, uint32_t count,
    zet_metric_group_handle_t *ptr_matched_group_handle) {
  ASSERT_NE(nullptr, *ptr_matched_group_handle);
  EXPECT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(
      lzt::get_default_context(), device, count, ptr_matched_group_handle));
}

void deactivate_metric_groups(ze_device_handle_t device) {
  EXPECT_ZE_RESULT_SUCCESS(zetContextActivateMetricGroups(
      lzt::get_default_context(), device, 0, nullptr));
}

void append_metric_query_begin(zet_command_list_handle_t commandList,
                               zet_metric_query_handle_t metricQueryHandle) {
  ASSERT_NE(nullptr, metricQueryHandle);
  EXPECT_ZE_RESULT_SUCCESS(
      zetCommandListAppendMetricQueryBegin(commandList, metricQueryHandle));
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
  EXPECT_ZE_RESULT_SUCCESS(zetCommandListAppendMetricQueryEnd(
      commandList, metricQueryHandle, eventHandle, numWaitEvents, waitEvents));
}

void append_metric_memory_barrier(zet_command_list_handle_t commandList) {
  EXPECT_ZE_RESULT_SUCCESS(
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
    break;
  }
}

void validate_metrics_common(
    zet_metric_group_handle_t hMetricGroup, const size_t rawDataSize,
    const uint8_t *rawData, bool requireOverflow, const char *metricNameForTest,
    std::vector<std::vector<zet_typed_value_t>> &typedValuesFound) {

  // Get set count and total metric value count
  uint32_t setCount = 0;
  uint32_t totalMetricValueCount = 0;
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupCalculateMultipleMetricValuesExp(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawDataSize, rawData, &setCount, &totalMetricValueCount, nullptr,
      nullptr));

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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_ZE_RESULT_SUCCESS(
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
        EXPECT_ZE_RESULT_SUCCESS(
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

  EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupCalculateMetricValues(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawDataSize, rawData, &metricValueCount, nullptr));

  // Get metric counts and metric values
  std::vector<zet_typed_value_t> metricValues(metricValueCount);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupCalculateMetricValues(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawDataSize, rawData, &metricValueCount, metricValues.data()));
  EXPECT_GT(metricValueCount, 0);

  // Setup
  uint32_t metricCount = 0;
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  LOG_DEBUG << "metricValueCount " << metricValueCount << " metricCount "
            << metricCount;

  for (uint32_t count = 0; count < metricValueCount; count++) {
    zet_metric_properties_t properties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
                                          nullptr};
    EXPECT_ZE_RESULT_SUCCESS(

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
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  std::vector<zet_metric_handle_t> phMetrics(metricCount);
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGet(hMetricGroup, &metricCount, phMetrics.data()));

  for (uint32_t metric = 0; metric < metricCount; metric++) {
    zet_metric_properties_t properties = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES,
                                          nullptr};
    EXPECT_ZE_RESULT_SUCCESS(
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
    EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupGetProperties(
        metric_group_handle, &metric_group_property));
    if (std::find(unique_domains.begin(), unique_domains.end(),
                  metric_group_property.domain) == unique_domains.end()) {
      unique_domains.push_back(metric_group_property.domain);
    }
  }

  // Collect metric groups for each domain
  for (auto &domain : unique_domains) {
    uint32_t metric_group_count = 0;
    for (auto metric_group_handle : metric_group_handles) {
      EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupGetProperties(
          metric_group_handle, &metric_group_property));
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

ze_result_t metric_calculate_metric_values_from_raw_data(
    zet_metric_group_handle_t hMetricGroup, std::vector<uint8_t> &rawData,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets) {

  uint32_t setCount = 0;
  uint32_t totalMetricValueCount = 0;

  EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupCalculateMultipleMetricValuesExp(
      hMetricGroup, ZET_METRIC_GROUP_CALCULATION_TYPE_METRIC_VALUES,
      rawData.size(), rawData.data(), &setCount, &totalMetricValueCount,
      nullptr, nullptr));

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

  EXPECT_GT(totalMetricValueCount, 0);
  return result;
}

void metric_get_metric_handles_from_metric_group(
    zet_metric_group_handle_t hMetricGroup,
    std::vector<zet_metric_handle_t> &hMetrics) {

  uint32_t metricCount = 0;
  EXPECT_ZE_RESULT_SUCCESS(zetMetricGet(hMetricGroup, &metricCount, nullptr));
  EXPECT_GT(metricCount, 0);

  hMetrics.resize(metricCount);
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricGet(hMetricGroup, &metricCount, hMetrics.data()));

  LOG_INFO << "number of metrics in metric group: metricCount " << metricCount;
}

void metric_get_metric_properties_for_metric_group(
    std::vector<zet_metric_handle_t> &metricHandles,
    std::vector<zet_metric_properties_t> &metricProperties) {

  for (uint32_t metric = 0; metric < metricHandles.size(); metric++) {
    metricProperties[metric] = {ZET_STRUCTURE_TYPE_METRIC_PROPERTIES, nullptr};
    EXPECT_ZE_RESULT_SUCCESS(zetMetricGetProperties(metricHandles[metric],
                                                    &metricProperties[metric]));
  }
}

void metric_validate_streamer_marker_data(
    std::vector<zet_metric_properties_t> &metricProperties,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets,
    std::vector<uint32_t> &streamerMarkerValues,
    uint32_t &streamer_marker_values_index) {

  uint32_t startIndex = 0;
  uint32_t validated_markers_count = 0;
  for (uint32_t dataIndex = 0; dataIndex < metricValueSets.size();
       dataIndex++) {

    const uint32_t metricCountForDataIndex = metricValueSets[dataIndex];
    const uint32_t reportCount =
        metricCountForDataIndex / metricProperties.size();
    uint32_t streamer_marker_values_index_per_set =
        streamer_marker_values_index;

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
          if (streamer_marker_values_index_per_set <
              streamerMarkerValues.size()) {
            LOG_INFO << "Valid Streamer Marker found with value: "
                     << metricTypedValue.value.ui64;
            EXPECT_EQ(
                streamerMarkerValues.at(streamer_marker_values_index_per_set),
                metricTypedValue.value.ui64);
            streamer_marker_values_index_per_set++;
          } else {
            FAIL() << "totalMetricValues contains more markers than those "
                      "appended";
          }
        }
      }
    }
    validated_markers_count += streamer_marker_values_index_per_set;
    startIndex += metricCountForDataIndex;
  }
  // Expecting that an equal number of markers have been validated for each set.
  EXPECT_FALSE(validated_markers_count % metricValueSets.size());
  streamer_marker_values_index =
      validated_markers_count / metricValueSets.size();
}

void generate_activatable_metric_group_list_for_device(
    ze_device_handle_t device,
    std::vector<metricGroupInfo_t> &metric_group_info_list,
    std::vector<zet_metric_group_handle_t> &activatable_metric_group_list) {
  std::map<uint32_t, zet_metric_group_handle_t> domain_map;

  for (auto &group_info : metric_group_info_list) {

    auto it = domain_map.find(group_info.domain);
    if (it == domain_map.end()) {
      domain_map.insert({group_info.domain, group_info.metricGroupHandle});
      LOG_DEBUG << "adding metric group handle for "
                << group_info.metricGroupName
                << " to activatable metric group list";
    }
  }

  for (auto &map_entry : domain_map) {
    activatable_metric_group_list.push_back(map_entry.second);
  }
}

void collect_device_properties(ze_device_handle_t device,
                               std::string &device_description) {

  ze_device_properties_t device_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};

  zeDeviceGetProperties(device, &device_properties);

  device_description = "test device name: ";
  device_description.append(device_properties.name);
  device_description.append(" uuid ");
  device_description.append(lzt::to_string(device_properties.uuid));
  device_description.append(". ");
  if (device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
    device_description.append("test subdevice id ");
    device_description.append(std::to_string(device_properties.subdeviceId));
    device_description.append(". ");
  } else {
    device_description.append("test device is a root device.");
  }
}

void display_device_properties(ze_device_handle_t device) {
  std::string device_description;

  collect_device_properties(device, device_description);
  LOG_INFO << device_description;
}

void generate_device_list_with_activatable_metric_group_handles(
    std::vector<ze_device_handle_t> devices,
    zet_metric_group_sampling_type_flags_t sampling_type,
    std::vector<activatable_metric_group_handle_list_for_device_t>
        &activatable_metric_group_handles_with_devices_list) {

  LOG_DEBUG << "ENTER generate_activatable_devices_list";

  for (auto device : devices) {
    ze_result_t result;
    lzt::display_device_properties(device);

    auto metric_group_info =
        lzt::get_metric_group_info(device, sampling_type, true, true);
    if (metric_group_info.size() == 0) {
      continue;
    }

    metric_group_info = lzt::optimize_metric_group_info_list(metric_group_info);

    std::vector<zet_metric_group_handle_t> activatable_metric_group_handle_list;
    lzt::generate_activatable_metric_group_list_for_device(
        device, metric_group_info, activatable_metric_group_handle_list);

    if (activatable_metric_group_handle_list.size() == 0) {
      continue;
    }

    activatable_metric_group_handle_list_for_device_t list_entry;
    list_entry.device = device;
    list_entry.activatable_metric_group_handle_list =
        std::move(activatable_metric_group_handle_list);
    activatable_metric_group_handles_with_devices_list.push_back(list_entry);
  }
  LOG_DEBUG << "LEAVE generate_activatable_devices_list";
}

void generate_device_list_with_metric_programmable_handles(
    std::vector<ze_device_handle_t> devices,
    std::vector<metric_programmable_handle_list_for_device>
        &device_list_with_metric_programmable_handles,
    uint32_t metric_programmable_handle_limit) {

  LOG_DEBUG << "ENTER generate_device_list_with_metric_programmable_handles, "
               "metric_programmable_limit "
            << metric_programmable_handle_limit;

  for (auto device : devices) {
    ze_result_t result;
    std::string device_description;

    collect_device_properties(device, device_description);

    uint32_t metric_programmable_handle_count = 0;
    ASSERT_ZE_RESULT_SUCCESS(zetMetricProgrammableGetExp(
        device, &metric_programmable_handle_count, nullptr));

    if (metric_programmable_handle_count == 0) {
      LOG_INFO << "the device: " << device_description
               << "does not support metric programmable";
      continue;
    }

    std::vector<zet_metric_programmable_exp_handle_t>
        metric_programmable_handles(metric_programmable_handle_count);
    ASSERT_ZE_RESULT_SUCCESS(
        zetMetricProgrammableGetExp(device, &metric_programmable_handle_count,
                                    metric_programmable_handles.data()));

    uint32_t metric_programmable_handle_subset_size =
        metric_programmable_handle_count;
    if (metric_programmable_handle_limit) {
      metric_programmable_handle_subset_size = std::min(
          metric_programmable_handle_count, metric_programmable_handle_limit);
    }

    metric_programmable_handle_list_for_device list;
    list.device = device;
    list.device_description = std::move(device_description);
    list.metric_programmable_handle_count = metric_programmable_handle_count;
    list.metric_programmable_handles_for_device = {
        metric_programmable_handles.begin(),
        metric_programmable_handles.begin() +
            metric_programmable_handle_subset_size};

    device_list_with_metric_programmable_handles.push_back(list);
  }
  LOG_DEBUG << "LEAVE generate_device_list_with_metric_programmable_handles";
}

// Validate param_info type.
bool programmable_param_info_type_to_string(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &type_string) {

  bool param_info_type_is_valid = true;

  switch (param_info.type) {
  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_DISAGGREGATION:
    type_string = "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_DISAGGREGATION";
    break;

  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_LATENCY:
    type_string = "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_LATENCY";
    break;

  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_UTILIZATION:
    type_string = "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_"
                  "UTILIZATION";
    break;

  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_AVERAGE:
    type_string =
        "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_AVERAGE";
    break;

  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_RATE:
    type_string = "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_RATE";
    break;

  case ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_BYTES:
    type_string = "ZET_METRIC_PROGRAMMABLE_PARAM_TYPE_EXP_NORMALIZATION_BYTES";
    break;

  default:
    param_info_type_is_valid = false;
    break;
  }
  return param_info_type_is_valid;
}

// Validate param info valueInfoType, get value string
bool programmable_param_info_value_info_type_to_string(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &value_info_type_string,
    std::string &value_info_default_value_string) {

  std::ostringstream oss;
  bool entry_is_valid = true;

  switch (param_info.valueInfoType) {
  case ZET_VALUE_INFO_TYPE_EXP_UINT32:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_UINT32";
    oss << param_info.defaultValue.ui32;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_UINT64:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_UINT64";
    oss << param_info.defaultValue.ui64;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_FLOAT32:
    EXPECT_FALSE(std::isnan(param_info.defaultValue.fp32))
        << "zetMetricProgrammableGetParamInfoExp has returned a "
           "zet_metric_programmable_param_info_exp_t structure with a "
           "ZET_VALUE_INFO_TYPE_EXP_FLOAT32 type field that contains an "
           "fp32 value that is not a number";
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_FLOAT32";
    oss << param_info.defaultValue.fp32;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_FLOAT64:
    EXPECT_FALSE(std::isnan(param_info.defaultValue.fp64))
        << "zetMetricProgrammableGetParamInfoExp has returned a "
           "zet_metric_programmable_param_info_exp_t structure with a "
           "ZET_VALUE_INFO_TYPE_EXP_FLOAT64 type field that contains an "
           "fp64 value that is not a number";
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_FLOAT64";
    oss << param_info.defaultValue.fp64;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_BOOL8:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_BOOL8";
    oss << (param_info.defaultValue.b8 ? "TRUE" : "FALSE");
    break;

  case ZET_VALUE_INFO_TYPE_EXP_UINT8:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_UINT8";
    oss << param_info.defaultValue.ui32;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_UINT16:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_UINT16";
    oss << param_info.defaultValue.ui32;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_UINT64_RANGE:
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_UINT64_RANGE";
    oss << param_info.defaultValue.ui64;
    break;

  case ZET_VALUE_INFO_TYPE_EXP_FLOAT64_RANGE:
    EXPECT_FALSE(std::isnan(param_info.defaultValue.fp64))
        << "zetMetricProgrammableGetParamInfoExp has returned a "
           "zet_metric_programmable_param_info_exp_t structure with a "
           "ZET_VALUE_INFO_TYPE_EXP_FLOAT64_RANGE type field that "
           "contains an fp64 that is not a number";
    value_info_type_string = "ZET_VALUE_INFO_TYPE_EXP_FLOAT64_RANGE";
    oss << param_info.defaultValue.fp64;
    break;

  default:
    EXPECT_TRUE(false)
        << "metric programmable param_info value valueInfoType field has "
           "an improper value "
        << param_info.valueInfoType;
    entry_is_valid = false;
    value_info_type_string = "INVALID InfoType ";
    oss << "Invalid InfoType " << param_info.valueInfoType << "has no value";
    break;
  }

  value_info_default_value_string = oss.str();
  return entry_is_valid;
}

bool programmable_param_info_validate(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &type_string, std::string &value_info_type_string,
    std::string &value_info_type_default_value_string) {

  LOG_DEBUG << "ENTER programmable_param_info_validate";

  bool success = false;

  success = programmable_param_info_type_to_string(param_info, type_string);
  EXPECT_TRUE(success) << "param_info has invalid type string " << type_string;

  size_t name_length;
  name_length =
      strnlen(param_info.name, ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP);
  EXPECT_GT(name_length, 0)
      << "metric programmable parameter_info name string is zero length";
  EXPECT_LT(name_length, ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP)
      << "metric programmable  parameter_info name string length "
      << name_length
      << " is longer than the allowed "
         "ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP";

  success = programmable_param_info_value_info_type_to_string(
      param_info, value_info_type_string, value_info_type_default_value_string);
  EXPECT_TRUE(success) << "param_info valueInfoType or defaultValue fields are "
                          "invalid. valueInfoTYpestring: "
                       << value_info_type_string << " ValueInfoDefaultValue: "
                       << value_info_type_default_value_string;

  EXPECT_GT(param_info.valueInfoCount, 0)
      << "programmablemetric param_info valueInfoCount field has a value "
         "of zero, which "
         "is not valid";

  LOG_DEBUG << "LEAVE programmable_param_info_validate";
  return success;
}

void fetch_metric_programmable_exp_properties(
    zet_metric_programmable_exp_handle_t metric_programmable_handle,
    zet_metric_programmable_exp_properties_t &metric_programmable_properties) {
  LOG_DEBUG << "ENTER fetch_metric_programmable_exp_properties";
  metric_programmable_properties.stype =
      ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_EXP_PROPERTIES;
  metric_programmable_properties.pNext = nullptr;
  ASSERT_ZE_RESULT_SUCCESS(zetMetricProgrammableGetPropertiesExp(
      metric_programmable_handle, &metric_programmable_properties));
  LOG_DEBUG << "metric programmable properties ########";
  LOG_DEBUG << "name " << metric_programmable_properties.name;
  LOG_DEBUG << "description " << metric_programmable_properties.description;
  LOG_DEBUG << "component " << metric_programmable_properties.component;
  LOG_DEBUG << "samplingType " << metric_programmable_properties.samplingType;
  LOG_DEBUG << "tierNumber " << metric_programmable_properties.tierNumber;
  LOG_DEBUG << "domain " << metric_programmable_properties.domain;
  LOG_DEBUG << "sourceId " << metric_programmable_properties.sourceId;
  LOG_DEBUG << "#####";

  LOG_DEBUG << "LEAVE fetch_metric_programmable_exp_properties";
}

void generate_param_info_exp_list_from_metric_programmable(
    zet_metric_programmable_exp_handle_t metric_programmable_handle,
    std::vector<zet_metric_programmable_param_info_exp_t> &param_infos,
    uint32_t metric_programmable_param_info_limit) {

  ze_result_t result;

  LOG_DEBUG << "ENTER generate_param_info_exp_list_from_metric_Programmable, "
               "metric_programmable_param_info_limit "
               "metric_programmable_limit "
            << metric_programmable_param_info_limit;

  zet_metric_programmable_exp_properties_t metric_programmable_properties;
  fetch_metric_programmable_exp_properties(metric_programmable_handle,
                                           metric_programmable_properties);

  EXPECT_GT(metric_programmable_properties.parameterCount, 0)
      << "zet_metric_programmable_exp_properties_t 'parameterCount' has an "
         "invald value of zero.";

  param_infos.resize(metric_programmable_properties.parameterCount);

  for (auto &parameter_info : param_infos) {
    parameter_info.stype =
        ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_PARAM_INFO_EXP;
    parameter_info.pNext = nullptr;
  }

  ASSERT_ZE_RESULT_SUCCESS(zetMetricProgrammableGetParamInfoExp(
      metric_programmable_handle,
      &metric_programmable_properties.parameterCount, param_infos.data()));

  uint32_t metric_programmable_param_info_subset_size =
      metric_programmable_properties.parameterCount;

  if (metric_programmable_param_info_limit) {
    metric_programmable_param_info_subset_size =
        std::min(metric_programmable_param_info_limit,
                 metric_programmable_properties.parameterCount);
  }

  LOG_INFO << "metric programmable group name: '"
           << metric_programmable_properties.name << "' description: '"
           << metric_programmable_properties.description << "' parameterCount: "
           << metric_programmable_properties.parameterCount
           << " but testing only: "
           << metric_programmable_param_info_subset_size << " parameters";

  param_infos.resize(metric_programmable_param_info_subset_size);

  LOG_DEBUG << "LEAVE generate_param_info_exp_list_from_metric_Programmable";
}

static void print_value_info(zet_value_info_type_exp_t value_info_type,
                             zet_value_info_exp_t value_info) {
  LOG_DEBUG << "ENTER print_value_info";
  switch (value_info_type) {
  case ZET_VALUE_INFO_TYPE_EXP_UINT32:
    LOG_DEBUG << value_info.ui32;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_UINT64:
    LOG_DEBUG << value_info.ui64;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_FLOAT32:
    LOG_DEBUG << value_info.fp32;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_FLOAT64:
    LOG_DEBUG << value_info.fp64;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_BOOL8:
    LOG_DEBUG << (value_info.b8 ? "TRUE" : "FALSE");
    break;
  case ZET_VALUE_INFO_TYPE_EXP_UINT8:
    LOG_DEBUG << value_info.ui8;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_UINT16:
    LOG_DEBUG << value_info.ui16;
    break;
  case ZET_VALUE_INFO_TYPE_EXP_UINT64_RANGE:
    LOG_DEBUG << "[" << value_info.ui64Range.ui64Min << ","
              << value_info.ui64Range.ui64Max << "]";
    break;
  case ZET_VALUE_INFO_TYPE_EXP_FLOAT64_RANGE: {
    double minVal = 0;
    memcpy(&minVal, &value_info.ui64Range.ui64Min, sizeof(uint64_t));
    double maxVal = 0;
    memcpy(&maxVal, &value_info.ui64Range.ui64Max, sizeof(uint64_t));
    LOG_DEBUG << "[" << minVal << "," << maxVal << "]";
  } break;
  default:
    EXPECT_TRUE(false) << "We have found invalid value for param_info "
                          "zet_value_info_exp_t field";
    break;
  }
  LOG_DEBUG << "LEAVE print_value_info";
}

void generate_param_value_info_list_from_param_info(
    zet_metric_programmable_exp_handle_t &programmable_handle, uint32_t ordinal,
    uint32_t value_info_count, zet_value_info_type_exp_t value_info_type,
    bool include_value_info_desc) {

  LOG_DEBUG << "ENTER generate_param_value_info_list_from_param_info";

  ze_result_t result;

  auto value_info =
      std::make_unique<zet_metric_programmable_param_value_info_exp_t[]>(
          value_info_count);

  auto value_info_desc =
      std::make_unique<zet_metric_programmable_param_value_info_exp_t[]>(
          value_info_count);

  auto count = value_info_count;
  uint32_t index = 0;
  if (include_value_info_desc) {
    for (auto i = 0; i < static_cast<int>(count); i++) {
      value_info_desc[i].stype =
          ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_PARAM_VALUE_INFO_EXP;
      value_info_desc[i].pNext = nullptr;
    }
  }
  for (auto i = 0; i < static_cast<int>(count); i++) {
    value_info[i].stype =
        ZET_STRUCTURE_TYPE_METRIC_PROGRAMMABLE_PARAM_VALUE_INFO_EXP;
    value_info[i].pNext =
        include_value_info_desc ? &value_info_desc[i] : nullptr;
  }

  result = zetMetricProgrammableGetParamValueInfoExp(
      programmable_handle, ordinal, &count, value_info.get());
  EXPECT_EQ(result, ZE_RESULT_SUCCESS)
      << "an error occurred in the zetMetricProgrammableGetParamValueInfoExp() "
         "call, error code "
      << result;
  EXPECT_EQ(count, value_info_count)
      << "the number of zet_metric_programmable_param_value_info_exp_t "
         "returned: "
      << count << " , does not agree with the expected count "
      << value_info_count;
  LOG_DEBUG << "param_info valueInfoCount is " << value_info_count;

  for (index = 0; index < count; index++) {
    print_value_info(value_info_type, value_info[index].valueInfo);
    if (include_value_info_desc) {
      const auto description_len =
          strnlen(value_info_desc[index].description,
                  ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP);
      EXPECT_LT(description_len,
                ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP);
      LOG_DEBUG << "value info description: "
                << value_info_desc[index].description;
    }
  }
  LOG_DEBUG << "LEAVE generate_param_value_info_list_from_param_info";
}

void generate_metric_handles_list_from_param_values(
    zet_metric_programmable_exp_handle_t &metric_programmable_handle,
    std::string name, std::string description,
    std::vector<zet_metric_programmable_param_value_exp_t> &parameter_values,
    std::vector<zet_metric_handle_t> &metric_handles,
    uint32_t metric_handles_limit) {

  ze_result_t result;
  uint32_t metric_handles_count = 0;

  LOG_DEBUG << "ENTER generate_metric_handles_list_from_param_values, "
               "metric_handles_limit "
            << metric_handles_limit << " :metric_programmable name: " << name
            << " :metric_programmable description: " << description;

  ASSERT_ZE_RESULT_SUCCESS(zetMetricCreateFromProgrammableExp(
      metric_programmable_handle, parameter_values.data(),
      parameter_values.size(), name.c_str(), description.c_str(),
      &metric_handles_count, nullptr));
  LOG_INFO
      << "zetMetricCreateFromProgrammableExp returned metric handle count: "
      << metric_handles_count << "\n";

  if (metric_handles_count == 0) {
    LOG_INFO << "skipping metric programmable which has no metrics";
    LOG_DEBUG << "LEAVE generate_metric_list_from_param_values, skipping, has "
                 "no metrics";
    metric_handles.resize(0);
    return;
  }

  uint32_t metric_handles_subset_size = metric_handles_count;

  if (metric_handles_limit) {
    metric_handles_subset_size =
        std::min(metric_handles_subset_size, metric_handles_limit);
  }

  LOG_INFO << "limiting number of handles to " << metric_handles_subset_size;

  metric_handles.resize(metric_handles_subset_size);

  ASSERT_ZE_RESULT_SUCCESS(zetMetricCreateFromProgrammableExp(
      metric_programmable_handle, parameter_values.data(),
      parameter_values.size(), name.c_str(), description.c_str(),
      &metric_handles_subset_size, metric_handles.data()));

  for (uint32_t handle_index = 0; handle_index < metric_handles_subset_size;
       ++handle_index) {
    const zet_metric_handle_t metric_handle = metric_handles[handle_index];
    zet_metric_properties_t metric_properties = {};
    lzt::get_metric_properties(metric_handle, &metric_properties);
    LOG_DEBUG << "%%%%%%%%";
    LOG_DEBUG << "metric properties for index " << handle_index;
    LOG_DEBUG << "name " << metric_properties.name;
    LOG_DEBUG << "description " << metric_properties.description;
    LOG_DEBUG << "component " << metric_properties.component;
    LOG_DEBUG << "tier number " << metric_properties.tierNumber;
    LOG_DEBUG << "type "
              << lzt::metric_type_to_string(metric_properties.metricType);
    LOG_DEBUG << "value type "
              << lzt::metric_value_type_to_string(metric_properties.resultType);
    LOG_DEBUG << "result units " << metric_properties.resultUnits;
    LOG_DEBUG << "%%%%%%";
  }
  LOG_DEBUG << "LEAVE generate_metric_list_from_param_values, "
               "metric_handles_subset_size "
            << metric_handles_subset_size;
}

void destroy_metric_handles_list(
    std::vector<zet_metric_handle_t> &metric_handles) {

  LOG_DEBUG << "ENTER destroy_metric_handles_list, size "
            << metric_handles.size();

  for (auto metric_handle : metric_handles) {
    EXPECT_ZE_RESULT_SUCCESS(zetMetricDestroyExp(metric_handle));
  }

  metric_handles.resize(0);
  LOG_DEBUG << "LEAVE destroy_metric_handles_list";
}

void destroy_metric_group_handles_list(
    std::vector<zet_metric_group_handle_t> &metric_group_handles_list) {
  LOG_DEBUG << "ENTER destroy_metric_group_handles_list of size "
            << metric_group_handles_list.size();
  for (auto metric_group_handle : metric_group_handles_list) {
    ze_result_t result;

    EXPECT_ZE_RESULT_SUCCESS(zetMetricGroupDestroyExp(metric_group_handle));
  }
  metric_group_handles_list.resize(0);
  LOG_DEBUG << "LEAVE destroy_metric_group_handles_list";
}

void metric_tracer_create(
    zet_context_handle_t context_handle, zet_device_handle_t device_handle,
    uint32_t metric_group_count,
    zet_metric_group_handle_t *ptr_metric_group_handle,
    zet_metric_tracer_exp_desc_t *ptr_tracer_descriptor,
    ze_event_handle_t notification_event_handle,
    zet_metric_tracer_exp_handle_t *ptr_metric_tracer_handle) {
  LOG_DEBUG << "create tracer";
  ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerCreateExp(
      context_handle, device_handle, metric_group_count,
      ptr_metric_group_handle, ptr_tracer_descriptor, notification_event_handle,
      ptr_metric_tracer_handle));
  ASSERT_NE(nullptr, *ptr_metric_tracer_handle)
      << "zetMetricTracerCreateExp returned a NULL handle";
}

void metric_tracer_destroy(
    zet_metric_tracer_exp_handle_t metric_tracer_handle) {
  LOG_DEBUG << "destroy tracer";
  EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerDestroyExp(metric_tracer_handle));
}

void metric_tracer_enable(zet_metric_tracer_exp_handle_t metric_tracer_handle,
                          ze_bool_t synchronous) {
  LOG_DEBUG << "enable tracer";
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricTracerEnableExp(metric_tracer_handle, synchronous));
}

void metric_tracer_disable(zet_metric_tracer_exp_handle_t metric_tracer_handle,
                           ze_bool_t synchronous) {
  LOG_DEBUG << "disable tracer";
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricTracerDisableExp(metric_tracer_handle, synchronous));
}

size_t metric_tracer_read_data_size(
    zet_metric_tracer_exp_handle_t metric_tracer_handle) {
  size_t metric_size = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zetMetricTracerReadDataExp(metric_tracer_handle, &metric_size, nullptr));
  EXPECT_NE(0u, metric_size)
      << "zetMetricTracerReadDataExp reports that there are no "
         "metrics available to read";
  LOG_DEBUG << "raw data size = " << metric_size;
  return metric_size;
}

void metric_tracer_read_data(
    zet_metric_tracer_exp_handle_t metric_tracer_handle,
    std::vector<uint8_t> *ptr_metric_data) {
  EXPECT_NE(nullptr, ptr_metric_data);
  size_t metric_size = metric_tracer_read_data_size(metric_tracer_handle);
  EXPECT_NE(0u, metric_size);
  ptr_metric_data->resize(metric_size);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
      metric_tracer_handle, &metric_size, ptr_metric_data->data()));
}

void enable_metrics_runtime(ze_device_handle_t device) {
  EXPECT_NE(nullptr, device);
  EXPECT_ZE_RESULT_SUCCESS(zetDeviceEnableMetricsExp(device));
}

void disable_metrics_runtime(ze_device_handle_t device) {
  EXPECT_NE(nullptr, device);
  EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
}

void metric_decoder_create(
    zet_metric_tracer_exp_handle_t metric_tracer_handle,
    zet_metric_decoder_exp_handle_t *ptr_metric_decoder_handle) {
  LOG_DEBUG << "create tracer decoder";
  ASSERT_NE(nullptr, metric_tracer_handle);
  ASSERT_ZE_RESULT_SUCCESS(zetMetricDecoderCreateExp(
      metric_tracer_handle, ptr_metric_decoder_handle));
  ASSERT_NE(nullptr, *ptr_metric_decoder_handle)
      << "zetMetricDecoderCreateExp returned a NULL handle";
}

void metric_decoder_destroy(
    zet_metric_decoder_exp_handle_t metric_decoder_handle) {
  LOG_DEBUG << "destroy tracer decoder";
  EXPECT_NE(nullptr, metric_decoder_handle);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricDecoderDestroyExp(metric_decoder_handle));
}

uint32_t metric_decoder_get_decodable_metrics_count(
    zet_metric_decoder_exp_handle_t metric_decoder_handle) {
  uint32_t decodable_metric_count = 0;
  EXPECT_NE(nullptr, metric_decoder_handle);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
      metric_decoder_handle, &decodable_metric_count, nullptr));
  EXPECT_NE(0u, decodable_metric_count)
      << "zetMetricDecoderGetDecodableMetricsExp reports that there are no "
         "decodable metrics";
  LOG_DEBUG << "found " << decodable_metric_count << " decodable metrics";
  return decodable_metric_count;
}

void metric_decoder_get_decodable_metrics(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles) {
  uint32_t decodable_metric_count =
      metric_decoder_get_decodable_metrics_count(metric_decoder_handle);
  EXPECT_NE(0u, decodable_metric_count);
  ptr_decodable_metric_handles->resize(decodable_metric_count);
  EXPECT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
      metric_decoder_handle, &decodable_metric_count,
      ptr_decodable_metric_handles->data()));
}

void metric_tracer_decode_get_various_counts(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    size_t *ptr_raw_data_size, std::vector<uint8_t> *ptr_raw_data,
    uint32_t decodable_metric_count,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles,
    uint32_t *ptr_set_count, uint32_t *ptr_metric_entries_count) {
  EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerDecodeExp(
      metric_decoder_handle, ptr_raw_data_size, ptr_raw_data->data(),
      decodable_metric_count, ptr_decodable_metric_handles->data(),
      ptr_set_count, nullptr, ptr_metric_entries_count, nullptr));
  LOG_DEBUG << "decodable metric entry count: " << *ptr_metric_entries_count;
  LOG_DEBUG << "set count: " << *ptr_set_count;
}

void metric_tracer_decode(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    size_t *ptr_raw_data_size, std::vector<uint8_t> *ptr_raw_data,
    uint32_t decodable_metric_count,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles,
    uint32_t *ptr_set_count,
    std::vector<uint32_t> *ptr_metric_entries_count_per_set,
    uint32_t *ptr_metric_entries_count,
    std::vector<zet_metric_entry_exp_t> *ptr_metric_entries) {
  EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerDecodeExp(
      metric_decoder_handle, ptr_raw_data_size, ptr_raw_data->data(),
      decodable_metric_count, ptr_decodable_metric_handles->data(),
      ptr_set_count, ptr_metric_entries_count_per_set->data(),
      ptr_metric_entries_count, ptr_metric_entries->data()));
  LOG_DEBUG << "actual decoded metric entry count: "
            << *ptr_metric_entries_count;
  LOG_DEBUG << "raw data decoded: " << *ptr_raw_data_size << " bytes";
}

void get_metric_groups_supporting_dma_buf(
    const std::vector<zet_metric_group_handle_t> &metric_group_handles,
    zet_metric_group_sampling_type_flags_t sampling_type,
    std::vector<zet_metric_group_handle_t> &dma_buf_metric_group_handles) {
  for (auto metric_group_handle : metric_group_handles) {
    zet_metric_group_type_exp_t metric_group_type{};
    metric_group_type.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_TYPE_EXP;
    metric_group_type.pNext = nullptr;
    metric_group_type.type = ZET_METRIC_GROUP_TYPE_EXP_FLAG_FORCE_UINT32;
    zet_metric_group_properties_t metric_group_properties = {};
    metric_group_properties.pNext = &metric_group_type;
    ASSERT_ZE_RESULT_SUCCESS(zetMetricGroupGetProperties(
        metric_group_handle, &metric_group_properties));
    if (metric_group_properties.samplingType == sampling_type &&
        metric_group_type.type ==
            ZET_METRIC_GROUP_TYPE_EXP_FLAG_EXPORT_DMA_BUF) {
      dma_buf_metric_group_handles.push_back(metric_group_handle);
    }
  }
}

void metric_get_dma_buf_fd_and_size(
    zet_metric_group_handle_t metric_group_handle, int &fd, size_t &size) {
  zet_export_dma_buf_exp_properties_t dma_buf_properties{};
  dma_buf_properties.stype = ZET_STRUCTURE_TYPE_EXPORT_DMA_EXP_PROPERTIES;
  dma_buf_properties.pNext = nullptr;
  dma_buf_properties.fd = -1;
  dma_buf_properties.size = 0;

  zet_metric_group_properties_t metric_group_properties = {};
  metric_group_properties.pNext = &dma_buf_properties;

  ASSERT_ZE_RESULT_SUCCESS(zetMetricGroupGetProperties(
      metric_group_handle, &metric_group_properties));
  ASSERT_NE(-1, dma_buf_properties.fd)
      << "metric group " << metric_group_properties.name
      << " is of type dma buf and cannot get the fd";
  ASSERT_NE(0, dma_buf_properties.size)
      << "metric group " << metric_group_properties.name
      << " is of type dma buf and cannot get the size";
  fd = dma_buf_properties.fd;
  size = dma_buf_properties.size;
  LOG_DEBUG << "metric group " << metric_group_properties.name
            << " supports dma buf, fd = " << fd << ", size = " << size;
}

void *metric_map_dma_buf_fd_to_memory(ze_device_handle_t device,
                                      ze_context_handle_t context, int fd,
                                      size_t size, size_t alignment) {
  void *mem_ret = nullptr;
  ze_external_memory_import_fd_t mem_import = {};
  mem_import.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_FD;
  mem_import.pNext = nullptr;
  mem_import.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  mem_import.fd = fd;
  ze_device_mem_alloc_desc_t mem_desc = {};
  mem_desc.stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC;
  mem_desc.pNext = &mem_import;
  mem_desc.flags = 0;
  mem_desc.ordinal = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zeMemAllocDevice(context, &mem_desc, size, alignment, device, &mem_ret));
  ze_memory_allocation_properties_t prop = {};
  prop.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  prop.pNext = NULL;
  EXPECT_ZE_RESULT_SUCCESS(
      zeMemGetAllocProperties(context, mem_ret, &prop, NULL));
  LOG_DEBUG << "zeMemGetAllocProperties returned memory: " << mem_ret;
  return mem_ret;
}

} // namespace level_zero_tests
