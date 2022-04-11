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

  std::vector<std::string> groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, true);
  zet_metric_group_handle_t matchedGroupHandle;
  std::string groupName = groupNameList[0];

  zetMetricQueryTest() {
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

class zetMetricQueryLoadTest
    : public ::testing::TestWithParam<
          std::tuple<ze_device_handle_t, std::string>> {
public:
  ze_device_handle_t device;
  zet_metric_group_handle_t matchedGroupHandle;
  zet_metric_query_pool_handle_t metricQueryPoolHandle;
  zet_metric_query_handle_t metricQueryHandle;
  zet_command_list_handle_t commandList;
  ze_command_queue_handle_t commandQueue;

  zetMetricQueryLoadTest() {
    device = std::get<0>(GetParam());
    std::string metricGroupName = std::get<1>(GetParam());
    commandList = lzt::create_command_list(device);
    commandQueue = lzt::create_command_queue(device);
    matchedGroupHandle =
        lzt::find_metric_group(device, metricGroupName,
                               ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    metricQueryPoolHandle = lzt::create_metric_query_pool_for_device(
        device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
        matchedGroupHandle);
    EXPECT_NE(nullptr, metricQueryPoolHandle);
    metricQueryHandle = lzt::metric_query_create(metricQueryPoolHandle);
  }
  ~zetMetricQueryLoadTest() {
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
};

TEST_P(
    zetMetricQueryLoadTest,
    GivenValidMetricGroupWhenEventBasedQueryIsCreatedThenExpectQueryToSucceed) {

  lzt::activate_metric_groups(device, 1, matchedGroupHandle);
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
  lzt::validate_metrics(matchedGroupHandle,
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
}

INSTANTIATE_TEST_CASE_P(
    parameterizedMetricQueryTests, zetMetricQueryLoadTest,
    ::testing::Combine(::testing::ValuesIn(lzt::get_metric_test_device_list()),
                       ::testing::ValuesIn(lzt::get_metric_group_name_list(
                           lzt::zeDevice::get_instance()->get_device(),
                           ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED,
                           true))));

class zetMetricStreamerTest : public ::testing::TestWithParam<
                                  std::tuple<ze_device_handle_t, std::string>> {
protected:
  uint32_t notifyEveryNReports = 30000;
  uint32_t samplingPeriod = 1000000;
  ze_event_handle_t eventHandle;
  lzt::zeEventPool eventPool;
  ze_device_handle_t device;
  zet_metric_group_handle_t matchedGroupHandle;
  zet_command_list_handle_t commandList;
  ze_command_queue_handle_t commandQueue;

  zetMetricStreamerTest() {
    device = std::get<0>(GetParam());
    std::string groupName = std::get<1>(GetParam());
    commandList = lzt::create_command_list(device);
    commandQueue = lzt::create_command_queue(device);
    matchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
    EXPECT_NE(nullptr, matchedGroupHandle);
    eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_DEVICE,
                           ZE_EVENT_SCOPE_FLAG_HOST);
  }
  ~zetMetricStreamerTest() {
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
    eventPool.destroy_event(eventHandle);
  }
};
TEST_P(
    zetMetricStreamerTest,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedThenExpectStreamerToSucceed) {

  lzt::activate_metric_groups(device, 1, matchedGroupHandle);
  zet_metric_streamer_handle_t metricStreamerHandle =
      lzt::metric_streamer_open_for_device(device, matchedGroupHandle,
                                           eventHandle, notifyEveryNReports,
                                           samplingPeriod);

  void *a_buffer, *b_buffer, *c_buffer;
  ze_group_count_t tg;
  ze_kernel_handle_t function =
      load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);
  zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                  nullptr);

  lzt::close_command_list(commandList);
  lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
  lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));

  std::vector<uint8_t> rawData;
  lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
  lzt::validate_metrics(
      matchedGroupHandle,
      lzt::metric_streamer_read_data_size(metricStreamerHandle),
      rawData.data());

  lzt::deactivate_metric_groups(device);
  lzt::metric_streamer_close(metricStreamerHandle);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
}

INSTANTIATE_TEST_CASE_P(
    parameterizedMetricStreamerTests, zetMetricStreamerTest,
    ::testing::Combine(::testing::ValuesIn(lzt::get_metric_test_device_list()),
                       ::testing::ValuesIn(lzt::get_metric_group_name_list(
                           lzt::zeDevice::get_instance()->get_device(),
                           ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED,
                           true))));

using zetMetricStreamerAppendMarkerTest = zetMetricStreamerTest;

TEST_P(
    zetMetricStreamerAppendMarkerTest,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedWithAppendStreamerMarkerThenExpectStreamerToSucceed) {

  lzt::activate_metric_groups(device, 1, matchedGroupHandle);
  zet_metric_streamer_handle_t metricStreamerHandle =
      lzt::metric_streamer_open_for_device(device, matchedGroupHandle,
                                           eventHandle, notifyEveryNReports,
                                           samplingPeriod);

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
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));

  std::vector<uint8_t> rawData;
  lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
  lzt::validate_metrics(
      matchedGroupHandle,
      lzt::metric_streamer_read_data_size(metricStreamerHandle),
      rawData.data());

  lzt::deactivate_metric_groups(device);
  lzt::metric_streamer_close(metricStreamerHandle);
  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
}

INSTANTIATE_TEST_CASE_P(
    parameterizedMetricStreamerTests, zetMetricStreamerAppendMarkerTest,
    ::testing::Combine(::testing::ValuesIn(lzt::get_metric_test_device_list()),
                       ::testing::ValuesIn(lzt::get_metric_group_name_list(
                           lzt::zeDevice::get_instance()->get_device(),
                           ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED,
                           true))));

} // namespace
