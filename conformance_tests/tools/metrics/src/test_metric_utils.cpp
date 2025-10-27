/*
 *
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <chrono>
#include <ctime>
#include <thread>

using lzt::to_nanoseconds;

using lzt::to_u32;

ze_kernel_handle_t get_matrix_multiplication_kernel(
    ze_device_handle_t device, ze_group_count_t *tg, void **a_buffer,
    void **b_buffer, void **c_buffer, uint32_t dimensions = 1024) {

  const char *dimensions_test_environment_variable =
      std::getenv("LZT_METRICS_MATRIX_MULTIPLICATION_DIMENSIONS");
  if (dimensions_test_environment_variable != nullptr) {
    dimensions = to_u32(dimensions_test_environment_variable);
    LOG_INFO
        << "overriding the matrix multiplication dimension as "
           "LZT_METRICS_MATRIX_MULTIPLICATION_DIMENSIONS is used with value of "
        << dimensions;
  }

  uint32_t m, k, n;
  m = k = n = dimensions;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));

  std::memcpy(*a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(*b_buffer, b.data(), b.size() * sizeof(float));

  tg->groupCountX = m / 16;
  tg->groupCountY = n / 16;
  tg->groupCountZ = 1;

  ze_module_handle_t module =
      lzt::create_module(device, "ze_matrix_multiplication_metrics.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(*a_buffer), a_buffer);
  lzt::set_argument_value(function, 1, sizeof(*b_buffer), b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(*c_buffer), c_buffer);
  return function;
}

void metric_validate_stall_sampling_data(
    std::vector<zet_metric_properties_t> &metricProperties,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets) {

  uint32_t ipOffset = UINT32_MAX;
  uint32_t activeOffset = UINT32_MAX;
  uint32_t controlStallOffset = UINT32_MAX;
  uint32_t pipeStallOffset = UINT32_MAX;
  uint32_t sendStallOffset = UINT32_MAX;
  uint32_t distStallOffset = UINT32_MAX;
  uint32_t sbidStallOffset = UINT32_MAX;
  uint32_t syncStallOffset = UINT32_MAX;
  uint32_t instrFetchStallOffset = UINT32_MAX;
  uint32_t otherStallOffset = UINT32_MAX;

  for (uint32_t i = 0; i < to_u32(metricProperties.size()); i++) {

    if (strcmp("IP", metricProperties[i].name) == 0) {
      ipOffset = i;
      continue;
    }
    if (strcmp("Active", metricProperties[i].name) == 0) {
      activeOffset = i;
      continue;
    }
    if (strcmp("ControlStall", metricProperties[i].name) == 0) {
      controlStallOffset = i;
      continue;
    }
    if (strcmp("PipeStall", metricProperties[i].name) == 0) {
      pipeStallOffset = i;
      continue;
    }
    if (strcmp("SendStall", metricProperties[i].name) == 0) {
      sendStallOffset = i;
      continue;
    }
    if (strcmp("DistStall", metricProperties[i].name) == 0) {
      distStallOffset = i;
      continue;
    }
    if (strcmp("SbidStall", metricProperties[i].name) == 0) {
      sbidStallOffset = i;
      continue;
    }
    if (strcmp("SyncStall", metricProperties[i].name) == 0) {
      syncStallOffset = i;
      continue;
    }
    if (strcmp("InstrFetchStall", metricProperties[i].name) == 0) {
      instrFetchStallOffset = i;
      continue;
    }
    if (strcmp("OtherStall", metricProperties[i].name) == 0) {
      otherStallOffset = i;
      continue;
    }
  }

  uint64_t IpAddress = 0;
  uint64_t ActiveCount = 0;
  uint64_t ControlStallCount = 0;
  uint64_t PipeStallCount = 0;
  uint64_t SendStallCount = 0;
  uint64_t DistStallCount = 0;
  uint64_t SbidStallCount = 0;
  uint64_t SyncStallCount = 0;
  uint64_t InstrFetchStallCount = 0;
  uint64_t OtherStallCount = 0;

  uint32_t metricSetStartIndex = 0;

  EXPECT_GT(metricValueSets.size(), 0u);
  for (uint32_t metricValueSetIndex = 0;
       metricValueSetIndex < metricValueSets.size(); metricValueSetIndex++) {

    const uint32_t metricCountForDataIndex =
        metricValueSets[metricValueSetIndex];
    const uint32_t reportCount =
        to_u32(metricCountForDataIndex / metricProperties.size());

    LOG_INFO << "for metricValueSetIndex " << metricValueSetIndex
             << " metricCountForDataIndex " << metricCountForDataIndex
             << " reportCount " << reportCount;

    EXPECT_GT(reportCount, 1);

    uint64_t tmpStallCount;
    bool reportCompleteFlag;

    for (uint32_t report = 0; report < reportCount; report++) {

      tmpStallCount = 0;
      reportCompleteFlag = false;

      auto getStallCount = [&totalMetricValues](uint32_t metricReport,
                                                uint32_t metricPropertiesSize,
                                                uint32_t metricOffset,
                                                uint32_t metricStartIndex) {
        uint32_t metricIndex =
            metricReport * metricPropertiesSize + metricOffset;
        zet_typed_value_t metricTypedValue =
            totalMetricValues[metricStartIndex + metricIndex];
        uint64_t metricValue = metricTypedValue.value.ui64;
        return metricValue;
      };
      uint32_t metricPropsSize = to_u32(metricProperties.size());

      IpAddress =
          getStallCount(report, metricPropsSize, ipOffset, metricSetStartIndex);

      tmpStallCount = getStallCount(report, metricPropsSize, activeOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      ActiveCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, controlStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      ControlStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, pipeStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      PipeStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, sendStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      SendStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, distStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      DistStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, sbidStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      SbidStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, syncStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      SyncStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize,
                                    instrFetchStallOffset, metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      InstrFetchStallCount += tmpStallCount;

      tmpStallCount = getStallCount(report, metricPropsSize, otherStallOffset,
                                    metricSetStartIndex);
      reportCompleteFlag |= (tmpStallCount != 0);
      OtherStallCount += tmpStallCount;

      if (!reportCompleteFlag) {
        LOG_INFO << "Report number " << report << " with IP address "
                 << IpAddress << " has zero for all stall counts";
      }
    }

    metricSetStartIndex += metricCountForDataIndex;
  }

  LOG_DEBUG << "ActiveCount " << ActiveCount;
  LOG_DEBUG << "ControlStallCount " << ControlStallCount;
  LOG_DEBUG << "PipeStallCount " << PipeStallCount;
  LOG_DEBUG << "SendStallCount " << SendStallCount;
  LOG_DEBUG << "DistStallCount " << DistStallCount;
  LOG_DEBUG << "SbidStallCount " << SbidStallCount;
  LOG_DEBUG << "SyncStallCount " << SyncStallCount;
  LOG_DEBUG << "InstrFetchStallCount " << InstrFetchStallCount;
  LOG_DEBUG << "OtherStallCount " << OtherStallCount;
}

void metric_run_ip_sampling_with_validation(
    bool enableOverflow, const std::vector<ze_device_handle_t> &devices,
    uint32_t notifyEveryNReports, uint32_t samplingPeriod,
    uint32_t timeForNReportsComplete) {

  uint32_t numberOfFunctionCalls;
  if (enableOverflow) {
    numberOfFunctionCalls = 8;
  } else {
    numberOfFunctionCalls = 1;
  }

  typedef struct _function_data {
    ze_kernel_handle_t function;
    ze_group_count_t tg;
    void *a_buffer, *b_buffer, *c_buffer;
  } function_data_t;

  std::vector<function_data_t> functionDataBuf(numberOfFunctionCalls);

  for (auto device : devices) {

    ze_device_properties_t deviceProperties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    zeDeviceGetProperties(device, &deviceProperties);

    LOG_INFO << "test device name " << deviceProperties.name << " uuid "
             << lzt::to_string(deviceProperties.uuid);
    if (deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
      LOG_INFO << "test subdevice id " << deviceProperties.subdeviceId;
    } else {
      LOG_INFO << "test device is a root device";
    }

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);

    auto metricGroupInfo = lzt::get_metric_type_ip_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
    if (metricGroupInfo.size() == 0) {
      GTEST_SKIP()
          << "No IP metric groups are available to test on this platform";
    }
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      for (auto &fData : functionDataBuf) {
        fData.function = get_matrix_multiplication_kernel(
            device, &fData.tg, &fData.a_buffer, &fData.b_buffer,
            &fData.c_buffer, 4096);
        zeCommandListAppendLaunchKernel(commandList, fData.function, &fData.tg,
                                        nullptr, 0, nullptr);
      }

      lzt::close_command_list(commandList);
      std::chrono::steady_clock::time_point startTime =
          std::chrono::steady_clock::now();

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());

      std::chrono::steady_clock::time_point endTime =
          std::chrono::steady_clock::now();
      auto elapsedTime = to_nanoseconds(endTime - startTime);

      LOG_INFO << "elapsed time for workload completion " << elapsedTime
               << " time for NReports to complete " << timeForNReportsComplete;
      if (elapsedTime < timeForNReportsComplete) {
        LOG_WARNING << "elapsed time for workload completion is too short";
      }

      const char *sleep_in_buffer_overflow_test_environment_variable =
          std::getenv("LZT_METRICS_BUFFER_OVERFLOW_SLEEP_MS");

      if (sleep_in_buffer_overflow_test_environment_variable != nullptr) {
        uint32_t value =
            to_u32(sleep_in_buffer_overflow_test_environment_variable);
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
      }

      size_t rawDataSize = 0;
      std::vector<uint8_t> rawData;
      rawDataSize = lzt::metric_streamer_read_data_size(metricStreamerHandle,
                                                        notifyEveryNReports);
      EXPECT_GT(rawDataSize, 0);
      rawData.resize(rawDataSize);
      lzt::metric_streamer_read_data(metricStreamerHandle, notifyEveryNReports,
                                     rawDataSize, &rawData);
      lzt::validate_metrics(groupInfo.metricGroupHandle, rawDataSize,
                            rawData.data(), false);
      rawData.resize(rawDataSize);

      std::vector<zet_typed_value_t> metricValues;
      std::vector<uint32_t> metricValueSets;
      ze_result_t result =
          level_zero_tests::metric_calculate_metric_values_from_raw_data(
              groupInfo.metricGroupHandle, rawData, metricValues,
              metricValueSets);

      if (enableOverflow) {
        EXPECT_EQ(ZE_RESULT_WARNING_DROPPED_DATA, result);
      } else {
        EXPECT_ZE_RESULT_SUCCESS(result);
      }

      std::vector<zet_metric_handle_t> metricHandles;
      lzt::metric_get_metric_handles_from_metric_group(
          groupInfo.metricGroupHandle, metricHandles);
      std::vector<zet_metric_properties_t> metricProperties(
          metricHandles.size());
      lzt::metric_get_metric_properties_for_metric_group(metricHandles,
                                                         metricProperties);

      metric_validate_stall_sampling_data(metricProperties, metricValues,
                                          metricValueSets);

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);

      for (auto &fData : functionDataBuf) {
        lzt::destroy_function(fData.function);
        lzt::free_memory(fData.a_buffer);
        lzt::free_memory(fData.b_buffer);
        lzt::free_memory(fData.c_buffer);
      }

      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}
