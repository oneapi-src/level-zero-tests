/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
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
                      bool includeExpFeature, bool one_group_per_domain);

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

void metric_streamer_read_data(
    zet_metric_streamer_handle_t metricStreamerHandle, uint32_t reports,
    size_t &rawDataSize, std::vector<uint8_t> *metricData);

std::vector<zet_metric_group_handle_t>
get_metric_groups_with_different_domains(const ze_device_handle_t device,
                                         uint32_t metric_groups_per_domain);

ze_result_t metric_calculate_metric_values_from_raw_data(
    zet_metric_group_handle_t hMetricGroup, std::vector<uint8_t> &rawData,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets);

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
    std::vector<uint32_t> &streamerMarkerValues,
    uint32_t &streamer_marker_values_index);

void generate_activatable_metric_group_handles_for_device(
    ze_device_handle_t device,
    std::vector<metricGroupInfo_t> &metric_group_info_list,
    std::vector<zet_metric_group_handle_t>
        &activatable_metric_group_handle_list);

void display_device_properties(ze_device_handle_t device);

typedef struct activatable_metric_group_handle_list_for_device {
  ze_device_handle_t device;
  std::vector<zet_metric_group_handle_t> activatable_metric_group_handle_list;
} activatable_metric_group_handle_list_for_device_t;

void generate_device_list_with_activatable_metric_group_handles(
    std::vector<ze_device_handle_t> devices,
    zet_metric_group_sampling_type_flags_t sampling_type,
    std::vector<activatable_metric_group_handle_list_for_device_t>
        &device_list_with_activatable_metric_group_handles);

typedef struct metric_programmable_handle_list_for_device {
  ze_device_handle_t device;
  std::string device_description;
  uint32_t metric_programmable_handle_count;
  std::vector<zet_metric_programmable_exp_handle_t>
      metric_programmable_handles_for_device;
} metric_programmable_handle_list_for_device_t;

void generate_device_list_with_metric_programmable_handles(
    std::vector<ze_device_handle_t> devices,
    std::vector<metric_programmable_handle_list_for_device>
        &device_list_with_metric_programmable_handles,
    uint32_t metric_programmable_handle_limit);

bool verify_value_type(zet_value_type_t value_type);

bool verify_metric_type(zet_metric_type_t metric_type);

void get_metric_properties(zet_metric_handle_t hMetric,
                           zet_metric_properties_t *metric_properties);

// Validate param_info type field.
bool programmable_param_info_type_to_string(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &type_string);

// Validate param info valueInfoType, get value string
bool programmable_param_info_value_info_type_to_string(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &value_info_type_string,
    std::string &value_info_default_value_string);

// Validate param_info structures
bool programmable_param_info_validate(
    zet_metric_programmable_param_info_exp_t &param_info,
    std::string &type_string, std::string &value_info_type_string,
    std::string &value_info_type_default_value_string);

void generate_param_info_exp_list_from_metric_programmable(
    zet_metric_programmable_exp_handle_t metric_programmable_handle,
    std::vector<zet_metric_programmable_param_info_exp_t> &param_infos,
    uint32_t metric_programmable_info_limit);

void generate_param_value_info_list_from_param_info(
    zet_metric_programmable_exp_handle_t &programmable_handle, uint32_t ordinal,
    uint32_t value_info_count, zet_value_info_type_exp_t value_info_type,
    bool include_value_info_desc);

void fetch_metric_programmable_exp_properties(
    zet_metric_programmable_exp_handle_t metric_programmable_handle,
    zet_metric_programmable_exp_properties_t &metric_programmable_properties);

std::string metric_value_type_to_string(zet_value_type_t result_type);

std::string metric_type_to_string(zet_metric_type_t metric_type);

void generate_metric_handles_list_from_param_values(
    zet_metric_programmable_exp_handle_t &metric_programmable_handle,
    const std::string group_name_prefix, const std::string description,
    std::vector<zet_metric_programmable_param_value_exp_t> &parameter_values,
    std::vector<zet_metric_handle_t> &metric_handles,
    uint32_t metric_handles_limit);

void destroy_metric_handles_list(
    std::vector<zet_metric_handle_t> &metric_handles);

void destroy_metric_group_handles_list(
    std::vector<zet_metric_group_handle_t> &metric_group_handles_list);

std::vector<zet_metric_group_handle_t> get_one_metric_group_per_domain(
    ze_device_handle_t device,
    std::vector<zet_metric_group_handle_t> &metricGroupHandleList);

void metric_tracer_create(
    zet_context_handle_t context_handle, zet_device_handle_t device_handle,
    uint32_t metric_group_count,
    zet_metric_group_handle_t *ptr_metric_group_handle,
    zet_metric_tracer_exp_desc_t *ptr_tracer_descriptor,
    ze_event_handle_t notification_event_handle,
    zet_metric_tracer_exp_handle_t *ptr_metric_tracer_handle);

void metric_tracer_destroy(zet_metric_tracer_exp_handle_t metric_tracer_handle);

void metric_tracer_enable(zet_metric_tracer_exp_handle_t metric_tracer_handle,
                          ze_bool_t synchronous);

void metric_tracer_disable(zet_metric_tracer_exp_handle_t metric_tracer_handle,
                           ze_bool_t synchronous);

size_t metric_tracer_read_data_size(
    zet_metric_tracer_exp_handle_t metric_tracer_handle);

void metric_tracer_read_data(
    zet_metric_tracer_exp_handle_t metric_tracer_handle,
    std::vector<uint8_t> *ptr_metric_data);

void enable_metrics_runtime(ze_device_handle_t device);

void disable_metrics_runtime(ze_device_handle_t device);

void metric_decoder_create(
    zet_metric_tracer_exp_handle_t metric_tracer_handle,
    zet_metric_decoder_exp_handle_t *ptr_metric_decoder_handle);

void metric_decoder_destroy(
    zet_metric_decoder_exp_handle_t metric_decoder_handle);

uint32_t metric_decoder_get_decodable_metrics_count(
    zet_metric_decoder_exp_handle_t metric_decoder_handle);

void metric_decoder_get_decodable_metrics(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles);

void metric_tracer_decode_get_various_counts(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    size_t *ptr_raw_data_size, std::vector<uint8_t> *ptr_raw_data,
    uint32_t decodable_metric_count,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles,
    uint32_t *ptr_set_count, uint32_t *ptr_metric_entries_count);

void metric_tracer_decode(
    zet_metric_decoder_exp_handle_t metric_decoder_handle,
    size_t *ptr_raw_data_size, std::vector<uint8_t> *ptr_raw_data,
    uint32_t decodable_metric_count,
    std::vector<zet_metric_handle_t> *ptr_decodable_metric_handles,
    uint32_t *ptr_set_count,
    std::vector<uint32_t> *ptr_metric_entries_count_per_set,
    uint32_t *ptr_metric_entries_count,
    std::vector<zet_metric_entry_exp_t> *ptr_metric_entries);

void get_metric_groups_supporting_dma_buf(
    const std::vector<zet_metric_group_handle_t> &metric_group_handles,
    zet_metric_group_sampling_type_flags_t sampling_type,
    std::vector<zet_metric_group_handle_t> &dma_buf_metric_group_handles);

void metric_get_dma_buf_fd_and_size(
    zet_metric_group_handle_t metric_group_handle, int &fd, size_t &size);

void *metric_map_dma_buf_fd_to_memory(ze_device_handle_t device,
                                      ze_context_handle_t context, int fd,
                                      size_t size, size_t alignment);

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_SYSMAN_METRIC_HPP */
