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

namespace level_zero_tests {

uint32_t get_metric_group_handles_count(ze_device_handle_t device);
std::vector<zet_metric_group_handle_t>
get_metric_group_handles(ze_device_handle_t device);
std::vector<zet_metric_group_properties_t>
get_metric_group_properties(std::vector<zet_metric_group_handle_t> metricGroup);
std::vector<zet_metric_group_properties_t>
get_metric_group_properties(ze_device_handle_t device);
std::vector<std::string>
get_metric_group_name_list(ze_device_handle_t device,
                           zet_metric_group_sampling_type_flags_t samplingType);
zet_metric_group_handle_t find_metric_group(ze_device_handle_t device,
                                            std::string metricGroupToFind,
                                            uint32_t samplingType);
zet_metric_query_pool_handle_t
create_metric_query_pool(zet_metric_query_pool_desc_t metricQueryPoolDesc,
                         zet_metric_group_handle_t metricGroup);
zet_metric_query_pool_handle_t
create_metric_query_pool(uint32_t count, zet_metric_query_pool_type_t type,
                         zet_metric_group_handle_t metricGroup);
void destroy_metric_query_pool(
    zet_metric_query_pool_handle_t metric_query_pool_handle);

zet_metric_query_handle_t
metric_query_create(zet_metric_query_pool_handle_t metricQueryPoolHandle);

void destroy_metric_query(zet_metric_query_handle_t metricQueryHandle);

size_t metric_query_get_data_size(zet_metric_query_handle_t metricQueryHandle);
void metric_query_get_data(zet_metric_query_handle_t metricQueryHandle,
                           std::vector<uint8_t> *metricData);
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
}; // namespace level_zero_tests

#endif /* TEST_HARNESS_SYSMAN_METRIC_HPP */
