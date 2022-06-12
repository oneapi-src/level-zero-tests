/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

ze_kernel_handle_t load_gpu(ze_device_handle_t device, ze_group_count_t *tg,
                            void **a_buffer, void **b_buffer, void **c_buffer) {
  int m, k, n;
  m = k = n = 4096;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));

  std::memcpy(*a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(*b_buffer, b.data(), b.size() * sizeof(float));

  int group_count_x = m / 16;
  int group_count_y = n / 16;

  tg->groupCountX = group_count_x;
  tg->groupCountY = group_count_y;
  tg->groupCountZ = 1;

  ze_module_handle_t module =
      lzt::create_module(device, "ze_matrix_multiplication.spv",
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

class zetMetricGroupTest : public ::testing::Test {
protected:
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  zetMetricGroupTest() {}
  ~zetMetricGroupTest() {}
};

TEST_F(
    zetMetricGroupTest,
    GivenValidEventBasedMetricGroupWhenvalidGroupNameIsrequestedThenExpectMatchingMetricHandle) {

  std::vector<std::string> groupNameList;
  groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
  for (auto groupName : groupNameList) {
    zet_metric_group_handle_t testMatchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    EXPECT_NE(nullptr, testMatchedGroupHandle);
  }
}

TEST_F(
    zetMetricGroupTest,
    GivenValidTimeBasedMetricGroupWhenvalidGroupNameIsrequestedThenExpectMatchingMetricHandle) {

  std::vector<std::string> groupNameList;
  groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  for (auto groupName : groupNameList) {
    zet_metric_group_handle_t testMatchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
    EXPECT_NE(nullptr, testMatchedGroupHandle);
  }
}

TEST_F(
    zetMetricGroupTest,
    GivenValidMetricGroupWhenValidGroupNameIsRequestedThenExpectGroupActivationAndDeactivationToSucceed) {

  std::vector<zet_metric_group_handle_t> groupHandleList;
  groupHandleList = lzt::get_metric_group_handles(device);
  EXPECT_NE(0, groupHandleList.size());
  for (zet_metric_group_handle_t groupHandle : groupHandleList) {
    lzt::activate_metric_groups(device, 1, groupHandle);
    lzt::deactivate_metric_groups(device);
  }
}

TEST_F(zetMetricGroupTest,
       GivenValidMetricGroupWhenStreamerIsOpenedThenExpectStreamerToSucceed) {

  uint32_t notifyEveryNReports = 1000;
  uint32_t samplingPeriod = 40000;
  std::vector<std::string> groupNameList;
  groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  for (auto groupName : groupNameList) {
    zet_metric_group_handle_t groupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
    EXPECT_NE(nullptr, groupHandle);
    lzt::activate_metric_groups(device, 1, groupHandle);
    zet_metric_streamer_handle_t streamerHandle = lzt::metric_streamer_open(
        groupHandle, nullptr, notifyEveryNReports, samplingPeriod);
    EXPECT_NE(nullptr, streamerHandle);
    lzt::metric_streamer_close(streamerHandle);
    lzt::deactivate_metric_groups(device);
  }
}

class zetMetricQueryTest : public zetMetricGroupTest {
protected:
  zet_metric_query_pool_handle_t metricQueryPoolHandle;
  zet_metric_query_handle_t metricQueryHandle;

  std::vector<std::string> groupNameList;
  zet_metric_group_handle_t matchedGroupHandle;
  std::string groupName;

  zetMetricQueryTest() {

    groupNameList = lzt::get_metric_group_name_list(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
    groupName = groupNameList[0];

    matchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    EXPECT_NE(nullptr, matchedGroupHandle);
    metricQueryPoolHandle = lzt::create_metric_query_pool(
        1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE, matchedGroupHandle);
    EXPECT_NE(nullptr, metricQueryPoolHandle);
    metricQueryHandle = lzt::metric_query_create(metricQueryPoolHandle);
  }
  ~zetMetricQueryTest() {
    lzt::destroy_metric_query(metricQueryHandle);
    lzt::destroy_metric_query_pool(metricQueryPoolHandle);
  }
};

TEST_F(
    zetMetricQueryTest,
    GivenValidMetricQueryPoolWhenValidMetricGroupIsPassedThenExpectQueryhandle) {
  EXPECT_NE(nullptr, metricQueryHandle);
}

TEST_F(
    zetMetricQueryTest,
    GivenOnlyMetricQueryWhenCommandListIsCreatedThenExpectCommandListToExecuteSucessfully) {

  zet_command_list_handle_t commandList = lzt::create_command_list();
  lzt::activate_metric_groups(device, 1, matchedGroupHandle);
  lzt::append_metric_query_begin(commandList, metricQueryHandle);
  lzt::append_metric_query_end(commandList, metricQueryHandle, nullptr);
  lzt::close_command_list(commandList);
  ze_command_queue_handle_t commandQueue = lzt::create_command_queue();
  lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
  lzt::synchronize(commandQueue, UINT64_MAX);
  lzt::deactivate_metric_groups(device);
  lzt::destroy_command_queue(commandQueue);
  lzt::destroy_command_list(commandList);
}

class zetMetricQueryLoadTest : public ::testing::Test {
protected:
  std::vector<ze_device_handle_t> devices;

  void SetUp() override { devices = lzt::get_metric_test_device_list(); }
};

using zetMetricQueryLoadTestNoValidate = zetMetricQueryLoadTest;

TEST_F(
    zetMetricQueryLoadTestNoValidate,
    GivenValidMetricGroupWhenEventBasedQueryNoValidateIsCreatedThenExpectQueryToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      zet_metric_query_pool_handle_t metricQueryPoolHandle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);

      zet_metric_query_handle_t metricQueryHandle =
          lzt::metric_query_create(metricQueryPoolHandle);

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);
      lzt::append_metric_query_begin(commandList, metricQueryHandle);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;

      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);
      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::append_metric_query_end(commandList, metricQueryHandle, eventHandle);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));
      std::vector<uint8_t> rawData;

      lzt::metric_query_get_data(metricQueryHandle, &rawData);

      eventPool.destroy_event(eventHandle);
      lzt::destroy_metric_query(metricQueryHandle);
      lzt::destroy_metric_query_pool(metricQueryPoolHandle);

      lzt::deactivate_metric_groups(device);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);

      lzt::reset_command_list(commandList);
    }

    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

TEST_F(
    zetMetricQueryLoadTest,
    GivenValidMetricGroupWhenEventBasedQueryIsCreatedThenExpectQueryToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      zet_metric_query_pool_handle_t metricQueryPoolHandle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);

      zet_metric_query_handle_t metricQueryHandle =
          lzt::metric_query_create(metricQueryPoolHandle);

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);
      lzt::append_metric_query_begin(commandList, metricQueryHandle);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;

      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);
      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::append_metric_query_end(commandList, metricQueryHandle, eventHandle);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));

      std::vector<uint8_t> rawData;
      lzt::metric_query_get_data(metricQueryHandle, &rawData);
      lzt::validate_metrics(groupInfo.metricGroupHandle,
                            lzt::metric_query_get_data_size(metricQueryHandle),
                            rawData.data());

      eventPool.destroy_event(eventHandle);
      lzt::destroy_metric_query(metricQueryHandle);
      lzt::destroy_metric_query_pool(metricQueryPoolHandle);

      lzt::deactivate_metric_groups(device);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);

      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

class zetMetricStreamerTest : public ::testing::Test {
protected:
  std::vector<ze_device_handle_t> devices;

  uint32_t notifyEveryNReports = 3000;
  uint32_t samplingPeriod = 1000000;
  ze_device_handle_t device;

  void SetUp() { devices = lzt::get_metric_test_device_list(); }
};

using zetMetricStreamerTestNoValidate = zetMetricStreamerTest;

TEST_F(
    zetMetricStreamerTestNoValidate,
    GivenValidMetricGroupWhenTimerBasedStreamerNoValidateIsCreatedThenExpectStreamerToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
      ze_result_t eventResult;
      eventResult = zeEventQueryStatus(eventHandle);

      if (ZE_RESULT_SUCCESS == eventResult) {
        size_t oneReportSize, allReportsSize, numReports;
        oneReportSize =
            lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
        allReportsSize = lzt::metric_streamer_read_data_size(
            metricStreamerHandle, UINT32_MAX);
        LOG_INFO << "Event triggered. Single report size: " << oneReportSize
                 << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY != eventResult) {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);

      lzt::deactivate_metric_groups(device);
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

TEST_F(
    zetMetricStreamerTest,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedThenExpectStreamerToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
      ze_result_t eventResult;
      eventResult = zeEventQueryStatus(eventHandle);

      if (ZE_RESULT_SUCCESS == eventResult) {
        size_t oneReportSize, allReportsSize, numReports;
        oneReportSize =
            lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
        allReportsSize = lzt::metric_streamer_read_data_size(
            metricStreamerHandle, UINT32_MAX);
        LOG_INFO << "Event triggered. Single report size: " << oneReportSize
                 << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY != eventResult) {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::deactivate_metric_groups(device);
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

using zetMetricStreamerAppendMarkerTestNoValidate = zetMetricStreamerTest;

TEST_F(
    zetMetricStreamerAppendMarkerTestNoValidate,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedWithAppendStreamerMarkerNoValidateThenExpectStreamerToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);

      uint32_t streamerMarker = 0;
      lzt::commandlist_append_streamer_marker(commandList, metricStreamerHandle,
                                              ++streamerMarker);
      lzt::append_barrier(commandList);
      lzt::commandlist_append_streamer_marker(commandList, metricStreamerHandle,
                                              ++streamerMarker);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
      ze_result_t eventResult;
      eventResult = zeEventQueryStatus(eventHandle);

      if (ZE_RESULT_SUCCESS == eventResult) {
        size_t oneReportSize, allReportsSize, numReports;
        oneReportSize =
            lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
        allReportsSize = lzt::metric_streamer_read_data_size(
            metricStreamerHandle, UINT32_MAX);
        LOG_INFO << "Event triggered. Single report size: " << oneReportSize
                 << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY != eventResult) {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);

      lzt::deactivate_metric_groups(device);
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

using zetMetricStreamerAppendMarkerTest = zetMetricStreamerTest;

TEST_F(
    zetMetricStreamerAppendMarkerTest,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedWithAppendStreamerMarkerThenExpectStreamerToSucceed) {

  for (auto device : devices) {

    ze_device_properties_t deviceProperties;
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function =
          load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);

      uint32_t streamerMarker = 0;
      lzt::commandlist_append_streamer_marker(commandList, metricStreamerHandle,
                                              ++streamerMarker);
      lzt::append_barrier(commandList);
      lzt::commandlist_append_streamer_marker(commandList, metricStreamerHandle,
                                              ++streamerMarker);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
      ze_result_t eventResult;
      eventResult = zeEventQueryStatus(eventHandle);

      if (ZE_RESULT_SUCCESS == eventResult) {
        size_t oneReportSize, allReportsSize, numReports;
        oneReportSize =
            lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
        allReportsSize = lzt::metric_streamer_read_data_size(
            metricStreamerHandle, UINT32_MAX);
        LOG_INFO << "Event triggered. Single report size: " << oneReportSize
                 << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY != eventResult) {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::deactivate_metric_groups(device);
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

} // namespace
