/*
 *
 * Copyright (C) 2019 Intel Corporation
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
  uint32_t domain = 0;

  metricGroupInfo(zet_metric_group_handle_t handle, std::string name,
                  uint32_t domain)
      : metricGroupHandle(handle), metricGroupName(std::move(name)),
        domain(domain) {}
} metricGroupInfo_t;

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
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle);

void destroy_metric_query(zet_metric_query_handle_t metricQueryHandle);

size_t metric_query_get_data_size(zet_metric_query_handle_t metricQueryHandle);
void metric_query_get_data(zet_metric_query_handle_t metricQueryHandle,
                           std::vector<uint8_t> *metricData);
zet_metric_streamer_handle_t metric_streamer_open_for_device(
    ze_device_handle_t device, zet_metric_group_handle_t matchedGroupHandle,
    ze_event_handle_t eventHandle, uint32_t notifyEveryNReports,
    uint32_t samplingPeriod);
zet_metric_streamer_handle_t
metric_streamer_open(zet_metric_group_handle_t matchedGroupHandle,
                     ze_event_handle_t eventHandle,
                     uint32_t notifyEveryNReports, uint32_t samplingPeriod);
void metric_streamer_close(zet_metric_streamer_handle_t metricStreamerHandle);
void commandlist_append_streamer_marker(
    zet_command_list_handle_t commandList,
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t streamerMarker);
size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle);
size_t metric_streamer_read_data_size(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t);
void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle,
    std::vector<uint8_t> *metricData);
void activate_metric_groups(ze_device_handle_t device, uint32_t count,
                            zet_metric_group_handle_t matchedGroupHandle);
void deactivate_metric_groups(ze_device_handle_t device);
void append_metric_query_begin(zet_command_list_handle_t commandList,
                               zet_metric_query_handle_t metricQueryHandle);
void append_metric_query_end(zet_command_list_handle_t commandList,
                             zet_metric_query_handle_t metricQueryHandle,
                             ze_event_handle_t eventHandle);
void validate_metrics(zet_metric_group_handle_t matchedGroupHandle,
                      const size_t rawDataSize, const uint8_t *rawData);
void validate_metrics_std(zet_metric_group_handle_t matchedGroupHandle,
                          const size_t rawDataSize, const uint8_t *rawData);
// Consider 20% of the metric groups in each domain for test input as default
std::vector<metricGroupInfo_t> optimize_metric_group_info_list(
    std::vector<metricGroupInfo_t> &metricGroupInfoList,
    uint32_t percentOfMetricGroupForTest = 20);
}; // namespace level_zero_tests

#endif /* TEST_HARNESS_SYSMAN_METRIC_HPP */
