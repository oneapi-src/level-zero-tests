/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_SYSMAN_METRIC_HPP
#define TEST_HARNESS_SYSMAN_METRIC_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include <vector>
#include <string>

namespace level_zero_tests {

typedef struct metricGroupInfo {
  zet_metric_group_handle_t metricGroupHandle;
  std::string metricGroupName;
  std::string metricGroupDescription;
  uint32_t domain = 0;
  uint32_t metricCount = 0;

  metricGroupInfo(zet_metric_group_handle_t handle, std::string name,
                  std::string description, uint32_t domain, uint32_t count)
      : metricGroupHandle(handle), metricGroupName(std::move(name)),
        metricGroupDescription(std::move(description)), domain(domain),
        metricCount(count) {}
} metricGroupInfo_t;

void logMetricGroupDebugInfo(const metricGroupInfo_t &metricGroupInfo);

std::vector<metricGroupInfo_t> get_metric_type_ip_group_info(
    ze_device_handle_t device,
    zet_metric_group_sampling_type_flags_t metricSamplingType);

std::vector<metricGroupInfo_t>
get_metric_group_info(ze_device_handle_t device,
                      zet_metric_group_sampling_type_flags_t samplingType,
                      bool includeExpFeature);

uint32_t get_metric_group_handles_count(ze_device_handle_t device);
std::vector<zet_metric_group_handle_t>
get_metric_group_handles(ze_device_handle_t device);
std::vector<zet_metric_group_properties_t>
get_metric_group_properties(std::vector<zet_metric_group_handle_t> metricGroup);
std::vector<zet_metric_group_properties_t>
get_metric_group_properties(ze_device_handle_t device);
std::vector<ze_device_handle_t> get_metric_test_no_subdevices_list(void);
std::vector<ze_device_handle_t>
get_metric_test_device_list(uint32_t testSubDeviceCount = 1);
std::vector<std::string>
get_metric_group_name_list(ze_device_handle_t device,
                           zet_metric_group_sampling_type_flags_t samplingType,
                           bool includeExpFeature);
zet_metric_group_handle_t find_metric_group(ze_device_handle_t device,
                                            std::string metricGroupToFind,
                                            uint32_t samplingType);
zet_metric_query_pool_handle_t create_metric_query_pool_for_device(
    zet_metric_query_pool_desc_t metricQueryPoolDesc,
    zet_metric_group_handle_t metricGroup);
zet_metric_query_pool_handle_t
create_metric_query_pool(uint32_t count, zet_metric_query_pool_type_t type,
                         zet_metric_group_handle_t metricGroup);
zet_metric_query_pool_handle_t
create_metric_query_pool_for_device(ze_device_handle_t device, uint32_t count,
                                    zet_metric_query_pool_type_t type,
                                    zet_metric_group_handle_t metricGroup);
void destroy_metric_query_pool(
    zet_metric_query_pool_handle_t metric_query_pool_handle);

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle,
                    uint32_t index);

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle);

void destroy_metric_query(zet_metric_query_handle_t metricQueryHandle);
void reset_metric_query(zet_metric_query_handle_t &metricQueryHandle);

size_t metric_query_get_data_size(zet_metric_query_handle_t metricQueryHandle);
void metric_query_get_data(zet_metric_query_handle_t metricQueryHandle,
                           std::vector<uint8_t> *metricData);
zet_metric_streamer_handle_t metric_streamer_open_for_device(
    ze_device_handle_t device, zet_metric_group_handle_t matchedGroupHandle,
    ze_event_handle_t eventHandle, uint32_t &notifyEveryNReports,
    uint32_t &samplingPeriod);
zet_metric_streamer_handle_t
metric_streamer_open(zet_metric_group_handle_t matchedGroupHandle,
                     ze_event_handle_t eventHandle,
                     uint32_t notifyEveryNReports, uint32_t samplingPeriod);
void metric_streamer_close(zet_metric_streamer_handle_t metricStreamerHandle);
ze_result_t commandlist_append_streamer_marker(
    zet_command_list_handle_t commandList,
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t streamerMarker);
size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle);
size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t);
void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle,
    std::vector<uint8_t> *metricData);
void activate_metric_groups(
    ze_device_handle_t device, uint32_t count,
    zet_metric_group_handle_t *ptr_matched_group_handle);
void deactivate_metric_groups(ze_device_handle_t device);
void append_metric_query_begin(zet_command_list_handle_t commandList,
                               zet_metric_query_handle_t metricQueryHandle);
void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle);
void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle,
                             uint32_t numWaitEvents,
                             ze_event_handle_t *waitEvents);
void append_metric_memory_barrier(zet_command_list_handle_t commandList);
void validate_metrics(zet_metric_group_handle_t hMetricGroup,
                      const size_t rawDataSize, const uint8_t *rawData,
                      bool requireOverflow = false);

void validate_metrics(
    zet_metric_group_handle_t hMetricGroup, const size_t rawDataSize,
    const uint8_t *rawData, const char *metricNameForTest,
    std::vector<std::vector<zet_typed_value_t>> &typedValuesFoundPerDevice);

void validate_metrics_std(zet_metric_group_handle_t matchedGroupHandle,
                          const size_t rawDataSize, const uint8_t *rawData);
// Consider 20% of the metric groups in each domain for test input as default
std::vector<metricGroupInfo_t> optimize_metric_group_info_list(
    std::vector<metricGroupInfo_t> &metricGroupInfoList,
    uint32_t percentOfMetricGroupForTest = 20,
    const char *MetricGroupName = nullptr);

bool validateMetricsStructures(zet_metric_group_handle_t hMetricGroup);

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t &rawDataSize,
    std::vector<uint8_t> *metricData);

std::vector<zet_metric_group_handle_t>
get_metric_groups_with_different_domains(const ze_device_handle_t device,
                                         uint32_t metric_groups_per_domain);

void metric_calculate_metric_values_from_raw_data(
    zet_metric_group_handle_t hMetricGroup, std::vector<uint8_t> &rawData,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets, bool expect_overflow = false);

void metric_get_metric_handles_from_metric_group(
    zet_metric_group_handle_t hMetricGroup,
    std::vector<zet_metric_handle_t> &hMetrics);

void metric_get_metric_properties_for_metric_group(
    std::vector<zet_metric_handle_t> &metricHandles,
    std::vector<zet_metric_properties_t> &metricProperties);

void metric_validate_streamer_marker_data(
    std::vector<zet_metric_properties_t> &metricProperties,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets,
    std::vector<uint32_t> &streamerMarkerValues);

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_SYSMAN_METRIC_HPP */
