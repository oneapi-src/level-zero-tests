/*
 *
 * Copyright (C) 2020-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <ctime>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_metric_utils.hpp"

namespace lzt = level_zero_tests;
namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace {

class zetMetricGroupTest : public ::testing::Test {
protected:
  ze_device_handle_t device = lzt::zeDevice::get_instance()->get_device();

  zetMetricGroupTest() {}
  ~zetMetricGroupTest() {}

  void run_activate_deactivate_test(bool reactivate);
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
    GivenValidEventBasedMetricGroupWhenvalidGroupNameIsrequestedThenExpectMetricsValidationsToSucceed) {

  std::vector<std::string> groupNameList;
  groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
  for (auto groupName : groupNameList) {
    zet_metric_group_handle_t testMatchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED);
    EXPECT_NE(nullptr, testMatchedGroupHandle);
    EXPECT_TRUE(lzt::validateMetricsStructures(testMatchedGroupHandle));
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
    GivenValidTimeBasedMetricGroupWhenvalidGroupNameIsrequestedThenExpectMatchingMetricsValidationsToSucceed) {

  std::vector<std::string> groupNameList;
  groupNameList = lzt::get_metric_group_name_list(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  LOG_INFO << "groupNameList size " << groupNameList.size();
  for (auto groupName : groupNameList) {
    LOG_INFO << "testing metric groupName " << groupName;
    zet_metric_group_handle_t testMatchedGroupHandle = lzt::find_metric_group(
        device, groupName, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED);
    EXPECT_NE(nullptr, testMatchedGroupHandle);
    EXPECT_TRUE(lzt::validateMetricsStructures(testMatchedGroupHandle));
  }
}

TEST_F(
    zetMetricGroupTest,
    GivenValidMetricGroupWhenValidGroupNameIsRequestedThenExpectGroupActivationAndDeactivationToSucceed) {

  std::vector<zet_metric_group_handle_t> groupHandleList;
  groupHandleList = lzt::get_metric_group_handles(device);
  EXPECT_NE(0, groupHandleList.size());
  for (zet_metric_group_handle_t groupHandle : groupHandleList) {
    lzt::activate_metric_groups(device, 1, &groupHandle);
    lzt::deactivate_metric_groups(device);
  }
}

void zetMetricGroupTest::run_activate_deactivate_test(bool reactivate) {
  bool test_executed = false;
  std::vector<zet_metric_group_handle_t> groupHandleList;
  groupHandleList = lzt::get_metric_group_handles(device);
  EXPECT_NE(0, groupHandleList.size());
  if (groupHandleList.size() < 2) {
    LOG_INFO << "Not enough metric groups to test multiple groups activation";
    return;
  }
  zet_metric_group_properties_t metric_group_properties = {};
  metric_group_properties.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  metric_group_properties.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGetProperties(groupHandleList[0],
                                        &metric_group_properties));
  auto domain = metric_group_properties.domain;
  auto domain_2 = 0;
  std::vector<zet_metric_group_handle_t> test_handles{groupHandleList[0]};
  for (zet_metric_group_handle_t groupHandle : groupHandleList) {
    metric_group_properties = {};
    metric_group_properties.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    metric_group_properties.pNext = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricGroupGetProperties(
                                     groupHandle, &metric_group_properties));
    if (metric_group_properties.domain != domain) {
      domain_2 = metric_group_properties.domain;
      // lzt::get_metric_group_properties(groupHandle);
      test_handles.push_back(groupHandle);
      lzt::activate_metric_groups(device, 2, test_handles.data());
      lzt::deactivate_metric_groups(device);

      if (reactivate) {
        LOG_INFO << "Deactivating then reactivating single metric group";
        lzt::activate_metric_groups(device, 1, test_handles.data());
        lzt::activate_metric_groups(device, 2, test_handles.data());
      }
      test_executed = true;
      break;
    }
  }
  if (!test_executed) {
    GTEST_SKIP() << "Not enough metric groups in different domains";
  } else {
    LOG_INFO << "Domain 1: " << domain << " Domain 2: " << domain_2;
  }
}

TEST_F(
    zetMetricGroupTest,
    GivenMetricGroupsInDifferentDomainWhenValidGroupIsActivatedThenExpectGroupActivationAndDeactivationToSucceed) {

  run_activate_deactivate_test(false);
}

TEST_F(
    zetMetricGroupTest,
    GivenMetricGroupsInDifferentDomainWhenValidGroupIsActivatedThenExpectGroupReActivationToSucceed) {

  run_activate_deactivate_test(true);
}

TEST_F(
    zetMetricGroupTest,
    GivenActiveMetricGroupsWhenActivatingSingleMetricGroupThenPreviouslyActiveGroupsAreDeactivated) {

  std::vector<zet_metric_group_handle_t> groupHandleList;
  groupHandleList = lzt::get_metric_group_handles(device);
  EXPECT_NE(0, groupHandleList.size());

  if (groupHandleList.size() < 2) {
    GTEST_SKIP() << "Skipping test due to not enough metric groups";
  }

  zet_metric_group_properties_t metric_group_properties = {};
  metric_group_properties.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
  metric_group_properties.pNext = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetMetricGroupGetProperties(groupHandleList[0],
                                        &metric_group_properties));

  std::set<uint32_t> domains;
  domains.insert(metric_group_properties.domain);
  std::vector<zet_metric_group_handle_t> test_handles{groupHandleList[0]};

  for (zet_metric_group_handle_t groupHandle : groupHandleList) {
    metric_group_properties = {};
    metric_group_properties.stype = ZET_STRUCTURE_TYPE_METRIC_GROUP_PROPERTIES;
    metric_group_properties.pNext = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zetMetricGroupGetProperties(
                                     groupHandle, &metric_group_properties));
    if (domains.find(metric_group_properties.domain) == domains.end()) {
      domains.insert(metric_group_properties.domain);
      test_handles.push_back(groupHandle);
    }
  }

  if (domains.size() < 2) {
    GTEST_SKIP() << "Skipping test due to not enough domains";
  }

  LOG_DEBUG << "Activating all metrics groups selected for test";
  lzt::activate_metric_groups(device, test_handles.size(), test_handles.data());

  std::vector<zet_metric_group_handle_t> activeGroupHandleList;
  zet_metric_streamer_handle_t streamer;
  zet_metric_streamer_desc_t streamer_desc = {
      ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, 1000, 40000};

  LOG_DEBUG << "Verifying groups are active by attempting to open streamer";
  for (auto test_handle : test_handles) {
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zetMetricStreamerOpen(lzt::get_default_context(), device,
                                    test_handle, &streamer_desc, nullptr,
                                    &streamer));

    ASSERT_EQ(ZE_RESULT_SUCCESS, zetMetricStreamerClose(streamer));
  }

  LOG_DEBUG << "Activating only first group";
  lzt::activate_metric_groups(device, 1, &test_handles[0]);

  LOG_DEBUG << "Verify only first group is active";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zetMetricStreamerOpen(lzt::get_default_context(), device,
                                  test_handles[0], &streamer_desc, nullptr,
                                  &streamer));
  ASSERT_EQ(ZE_RESULT_SUCCESS, zetMetricStreamerClose(streamer));

  for (auto test_handle : test_handles) {
    if (test_handle == test_handles[0]) {
      continue;
    }
    ASSERT_NE(ZE_RESULT_SUCCESS,
              zetMetricStreamerOpen(lzt::get_default_context(), device,
                                    test_handle, &streamer_desc, nullptr,
                                    &streamer));
  }

  // deactivate all groups
  lzt::deactivate_metric_groups(device);
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
    lzt::activate_metric_groups(device, 1, &groupHandle);
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

TEST_F(zetMetricQueryTest,
       GivenValidMetricQueryHandleWhenResettingQueryHandleThenExpectSuccess) {
  lzt::reset_metric_query(metricQueryHandle);
}

TEST_F(
    zetMetricQueryTest,
    GivenOnlyMetricQueryWhenCommandListIsCreatedThenExpectCommandListToExecuteSucessfully) {

  zet_command_list_handle_t commandList = lzt::create_command_list();
  lzt::activate_metric_groups(device, 1, &matchedGroupHandle);
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      zet_metric_query_pool_handle_t metricQueryPoolHandle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);

      zet_metric_query_handle_t metricQueryHandle =
          lzt::metric_query_create(metricQueryPoolHandle);

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);
      lzt::append_metric_query_begin(commandList, metricQueryHandle);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;

      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);
      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
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

void run_test(const ze_device_handle_t &device, bool reset, bool immediate) {
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

  ze_command_queue_handle_t commandQueue = nullptr;
  ze_command_list_handle_t commandList = nullptr;
  if (immediate) {
    commandList = lzt::create_immediate_command_list(device);
  } else {
    commandQueue = lzt::create_command_queue(device);
    commandList = lzt::create_command_list(device);
  }
  auto metricGroupInfo = lzt::get_metric_group_info(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
  metricGroupInfo =
      lzt::optimize_metric_group_info_list(metricGroupInfo, reset ? 1 : 20);

  for (auto groupInfo : metricGroupInfo) {
    LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

    zet_metric_query_pool_handle_t metricQueryPoolHandle =
        lzt::create_metric_query_pool_for_device(
            device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
            groupInfo.metricGroupHandle);

    zet_metric_query_handle_t metricQueryHandle =
        lzt::metric_query_create(metricQueryPoolHandle);

    lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);
    lzt::append_metric_query_begin(commandList, metricQueryHandle);
    lzt::append_barrier(commandList, nullptr, 0, nullptr);
    ze_event_handle_t eventHandle;
    lzt::zeEventPool eventPool;

    eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                           ZE_EVENT_SCOPE_FLAG_HOST);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer);

    zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                    nullptr);
    lzt::append_barrier(commandList);
    lzt::append_metric_query_end(commandList, metricQueryHandle, eventHandle);

    lzt::close_command_list(commandList);
    if (!immediate) {
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
      lzt::synchronize(commandQueue, UINT64_MAX);
    }

    if (!immediate) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));
    } else {
      lzt::event_host_synchronize(eventHandle, UINT64_MAX);
    }

    std::vector<uint8_t> rawData;
    lzt::metric_query_get_data(metricQueryHandle, &rawData);
    lzt::validate_metrics(groupInfo.metricGroupHandle,
                          lzt::metric_query_get_data_size(metricQueryHandle),
                          rawData.data());
    if (reset && !immediate) {
      lzt::reset_metric_query(metricQueryHandle);

      lzt::reset_command_list(commandList);
      lzt::append_metric_query_begin(commandList, metricQueryHandle);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);

      // reset buffers
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      lzt::destroy_function(function);

      function = get_matrix_multiplication_kernel(device, &tg, &a_buffer,
                                                  &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      lzt::append_metric_query_end(commandList, metricQueryHandle, eventHandle);
      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));

      lzt::metric_query_get_data(metricQueryHandle, &rawData);
      lzt::validate_metrics(groupInfo.metricGroupHandle,
                            lzt::metric_query_get_data_size(metricQueryHandle),
                            rawData.data());
    }

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

  if (commandQueue) {
    lzt::destroy_command_queue(commandQueue);
  }
  lzt::destroy_command_list(commandList);
}

TEST_F(
    zetMetricQueryLoadTest,
    GivenValidMetricGroupWhenEventBasedQueryIsCreatedThenExpectQueryToSucceed) {

  for (auto &device : devices) {
    run_test(device, false, false);
  }
}

TEST_F(
    zetMetricQueryLoadTest,
    GivenWorkloadExecutedWithMetricQueryWhenResettingQueryHandleThenResetSucceedsAndCanReuseHandle) {
  for (auto &device : devices) {
    run_test(device, true, false);
  }
}

TEST_F(
    zetMetricQueryLoadTest,
    GivenWorkloadExecutedOnImmediateCommandListWhenQueryingThenQuerySucceeds) {

  for (auto &device : devices) {
    run_test(device, false, true);
  }
}

void run_multi_device_query_load_test(
    const std::vector<zet_device_handle_t> &devices) {

  LOG_INFO << "Testing multi device query load";

  if (devices.size() < 2) {
    GTEST_SKIP() << "Skipping test as less than 2 devices are available";
  }

  auto device_0 = devices[0];
  auto device_1 = devices[1];
  auto command_queue_0 = lzt::create_command_queue(device_0);
  auto command_queue_1 = lzt::create_command_queue(device_1);

  auto command_list_0 = lzt::create_command_list(device_0);
  auto command_list_1 = lzt::create_command_list(device_1);

  auto metric_group_info_0 = lzt::get_metric_group_info(
      device_0, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, true);
  auto metric_group_info_1 = lzt::get_metric_group_info(
      device_1, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, true);

  metric_group_info_0 =
      lzt::optimize_metric_group_info_list(metric_group_info_0);
  metric_group_info_1 =
      lzt::optimize_metric_group_info_list(metric_group_info_1);

  auto groupInfo_0 = metric_group_info_0[0];
  auto groupInfo_1 = metric_group_info_1[0];

  auto metricQueryPoolHandle_0 = lzt::create_metric_query_pool_for_device(
      device_0, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
      groupInfo_0.metricGroupHandle);

  auto metricQueryPoolHandle_1 = lzt::create_metric_query_pool_for_device(
      device_1, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
      groupInfo_1.metricGroupHandle);

  auto metricQueryHandle_0 = lzt::metric_query_create(metricQueryPoolHandle_0);
  auto metricQueryHandle_1 = lzt::metric_query_create(metricQueryPoolHandle_1);

  lzt::activate_metric_groups(device_0, 1, &groupInfo_0.metricGroupHandle);
  lzt::activate_metric_groups(device_1, 1, &groupInfo_1.metricGroupHandle);

  lzt::append_metric_query_begin(command_list_0, metricQueryHandle_0);
  lzt::append_metric_query_begin(command_list_1, metricQueryHandle_1);

  lzt::append_barrier(command_list_0, nullptr, 0, nullptr);
  lzt::append_barrier(command_list_1, nullptr, 0, nullptr);

  ze_event_handle_t event_handle_0, event_handle_1;
  lzt::zeEventPool event_pool;

  event_pool.create_event(event_handle_0, ZE_EVENT_SCOPE_FLAG_HOST,
                          ZE_EVENT_SCOPE_FLAG_HOST);
  event_pool.create_event(event_handle_1, ZE_EVENT_SCOPE_FLAG_HOST,
                          ZE_EVENT_SCOPE_FLAG_HOST);

  void *a_buffer_0, *b_buffer_0, *c_buffer_0;
  ze_group_count_t tg_0;

  void *a_buffer_1, *b_buffer_1, *c_buffer_1;
  ze_group_count_t tg_1;

  auto function_0 = get_matrix_multiplication_kernel(
      device_0, &tg_0, &a_buffer_0, &b_buffer_0, &c_buffer_0);

  auto function_1 = get_matrix_multiplication_kernel(
      device_1, &tg_1, &a_buffer_1, &b_buffer_1, &c_buffer_1);

  lzt::append_launch_function(command_list_0, function_0, &tg_0, event_handle_0,
                              0, nullptr);
  lzt::append_barrier(command_list_0);
  lzt::append_metric_query_end(command_list_0, metricQueryHandle_0,
                               event_handle_0);

  lzt::append_launch_function(command_list_1, function_1, &tg_1, event_handle_1,
                              0, nullptr);
  lzt::append_barrier(command_list_1);
  lzt::append_metric_query_end(command_list_1, metricQueryHandle_1,
                               event_handle_1);

  lzt::close_command_list(command_list_0);
  lzt::close_command_list(command_list_1);

  lzt::execute_command_lists(command_queue_0, 1, &command_list_0, nullptr);
  lzt::execute_command_lists(command_queue_1, 1, &command_list_1, nullptr);

  lzt::synchronize(command_queue_0, UINT64_MAX);
  lzt::synchronize(command_queue_1, UINT64_MAX);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_handle_0));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(event_handle_1));

  std::vector<uint8_t> raw_data_0, raw_data_1;

  lzt::metric_query_get_data(metricQueryHandle_0, &raw_data_0);
  lzt::metric_query_get_data(metricQueryHandle_1, &raw_data_1);

  event_pool.destroy_event(event_handle_0);
  event_pool.destroy_event(event_handle_1);

  lzt::validate_metrics(groupInfo_0.metricGroupHandle,
                        lzt::metric_query_get_data_size(metricQueryHandle_0),
                        raw_data_0.data());
  lzt::validate_metrics(groupInfo_1.metricGroupHandle,
                        lzt::metric_query_get_data_size(metricQueryHandle_1),
                        raw_data_1.data());

  lzt::destroy_metric_query(metricQueryHandle_0);
  lzt::destroy_metric_query(metricQueryHandle_1);
  lzt::destroy_metric_query_pool(metricQueryPoolHandle_0);
  lzt::destroy_metric_query_pool(metricQueryPoolHandle_1);

  lzt::deactivate_metric_groups(device_0);
  lzt::deactivate_metric_groups(device_1);

  lzt::destroy_function(function_0);
  lzt::destroy_function(function_1);

  lzt::free_memory(a_buffer_0);
  lzt::free_memory(a_buffer_1);
  lzt::free_memory(b_buffer_0);
  lzt::free_memory(b_buffer_1);
  lzt::free_memory(c_buffer_0);
  lzt::free_memory(c_buffer_1);

  lzt::destroy_command_list(command_list_0);
  lzt::destroy_command_list(command_list_1);
  lzt::destroy_command_queue(command_queue_0);
  lzt::destroy_command_queue(command_queue_1);
}

TEST_F(zetMetricQueryLoadTest,
       GivenValidMetricGroupsWhenMultipleDevicesQueryThenExpectQueryToSucceed) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_multi_device_query_load_test(devices);
}

TEST_F(
    zetMetricQueryLoadTest,
    GivenValidMetricGroupsWhenMultipleSubdevicesQueryThenExpectQueryToSucceed) {
  auto subdevices = lzt::get_all_sub_devices();

  run_multi_device_query_load_test(subdevices);
}

class zetMetricQueryLoadStdTest : public ::testing::Test {
protected:
  std::vector<ze_device_handle_t> devices;

  void SetUp() override { devices = lzt::get_metric_test_no_subdevices_list(); }
};

TEST_F(
    zetMetricQueryLoadStdTest,
    GivenValidMetricGroupWhenEventBasedQueryWithNoSubdevicesListIsCreatedThenExpectQueryAndSpecValidateToSucceed) {

  for (auto device : devices) {

    uint32_t subDeviceCount = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
    if (subDeviceCount != 0) {
      continue;
    }

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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo, 1);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      zet_metric_query_pool_handle_t metricQueryPoolHandle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);

      zet_metric_query_handle_t metricQueryHandle =
          lzt::metric_query_create(metricQueryPoolHandle);

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);
      lzt::append_metric_query_begin(commandList, metricQueryHandle);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;

      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);
      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::append_barrier(commandList, nullptr, 0, nullptr);
      lzt::append_metric_query_end(commandList, metricQueryHandle, eventHandle);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventQueryStatus(eventHandle));

      std::vector<uint8_t> rawData;
      lzt::metric_query_get_data(metricQueryHandle, &rawData);
      lzt::validate_metrics_std(
          groupInfo.metricGroupHandle,
          lzt::metric_query_get_data_size(metricQueryHandle), rawData.data());

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
  uint64_t TimeForNReportsComplete = notifyEveryNReports * samplingPeriod;
  ze_device_handle_t device;

  void SetUp() { devices = lzt::get_metric_test_device_list(); }
};

using zetMetricStreamerTestNoValidate = zetMetricStreamerTest;

TEST_F(
    zetMetricStreamerTestNoValidate,
    GivenValidMetricGroupWhenTimerBasedStreamerNoValidateIsCreatedThenExpectStreamerToSucceed) {

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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);
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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);
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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
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
    GivenValidTypeIpMetricGroupWhenTimerBasedStreamerIsCreatedAndOverflowTriggeredThenExpectStreamerValidateError) {

  const uint32_t numberOfFunctionCalls = 8;

  struct {
    ze_kernel_handle_t function;
    ze_group_count_t tg;
    void *a_buffer, *b_buffer, *c_buffer;
  } functionDataBuf[numberOfFunctionCalls];

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
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    if (metricGroupInfo.size() == 0) {
      LOG_INFO << "no IP metric groups are available to test on this platform";
    }

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      for (auto &fData : functionDataBuf) {
        fData.function = get_matrix_multiplication_kernel(
            device, &fData.tg, &fData.a_buffer, &fData.b_buffer,
            &fData.c_buffer, 8192);
        zeCommandListAppendLaunchKernel(commandList, fData.function, &fData.tg,
                                        nullptr, 0, nullptr);
      }

      lzt::close_command_list(commandList);

      std::chrono::steady_clock::time_point startTime =
          std::chrono::steady_clock::now();

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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::chrono::steady_clock::time_point endTime =
          std::chrono::steady_clock::now();

      uint64_t elapsedTime =
          std::chrono::duration_cast<std::chrono::nanoseconds>(endTime -
                                                               startTime)
              .count();

      LOG_INFO << "elapsed time for workload completion " << elapsedTime
               << " time for NReports to complete " << TimeForNReportsComplete;
      if (elapsedTime < TimeForNReportsComplete) {
        LOG_WARNING << "elapsed time for workload completion is too short";
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data(), true);

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

using zetMetricStreamerAppendMarkerTestNoValidate = zetMetricStreamerTest;

TEST_F(
    zetMetricStreamerAppendMarkerTestNoValidate,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedWithAppendStreamerMarkerNoValidateThenExpectStreamerToSucceed) {

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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);
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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);
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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
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

class zetMetricStreamerStdTest : public ::testing::Test {
protected:
  std::vector<ze_device_handle_t> devices;

  uint32_t notifyEveryNReports = 3000;
  uint32_t samplingPeriod = 1000000;
  ze_device_handle_t device;

  void SetUp() override { devices = lzt::get_metric_test_no_subdevices_list(); }
};

TEST_F(
    zetMetricStreamerStdTest,
    GivenValidMetricGroupWhenTimerBasedStreamerWithNoSubdevicesListIsCreatedWithAppendStreamerMarkerThenExpectStreamerAndSpecValidateToSucceed) {

  for (auto device : devices) {

    uint32_t subDeviceCount = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetSubDevices(device, &subDeviceCount, nullptr));
    if (subDeviceCount != 0) {
      continue;
    }

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

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo, 1);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);
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
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics_std(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
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

TEST(
    zetMetricStreamProcessTest,
    GivenWorkloadExecutingInSeparateProcessWhenStreamingSingleMetricsThenExpectValidMetrics) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  // setup monitor
  auto metricGroupInfo = lzt::get_metric_group_info(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

  if (metricGroupInfo.empty()) {
    LOG_INFO << "No metric groups found";
    return;
  }
  // pick a metric group
  auto groupInfo = metricGroupInfo[0];

  LOG_INFO << "Selected metric group: " << groupInfo.metricGroupName
           << " domain: " << groupInfo.domain;

  LOG_INFO << "Activating metric group: " << groupInfo.metricGroupName;
  lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

  ze_event_handle_t eventHandle;
  lzt::zeEventPool eventPool;
  eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                         ZE_EVENT_SCOPE_FLAG_HOST);

  uint32_t notifyEveryNReports = 3000;
  uint32_t samplingPeriod = 1000000;
  zet_metric_streamer_handle_t metricStreamerHandle =
      lzt::metric_streamer_open_for_device(device, groupInfo.metricGroupHandle,
                                           eventHandle, notifyEveryNReports,
                                           samplingPeriod);
  ASSERT_NE(nullptr, metricStreamerHandle);

  //================================================================================
  LOG_INFO << "Starting workload in separate process";
  fs::path helper_path(fs::current_path() / "metrics");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  fs::path helper = bp::search_path("test_metric_helper", paths);
  bp::child metric_helper(helper);

  // start monitor
  do {
    LOG_DEBUG << "Waiting for data (event synchronize)...";
    lzt::event_host_synchronize(eventHandle,
                                std::numeric_limits<uint64_t>::max());
    lzt::event_host_reset(eventHandle);

    // read data
    size_t oneReportSize, allReportsSize, numReports;
    oneReportSize =
        lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
    allReportsSize =
        lzt::metric_streamer_read_data_size(metricStreamerHandle, UINT32_MAX);

    LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
              << ". All reports size:" << allReportsSize;

    EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

    std::vector<uint8_t> rawData;
    lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
    lzt::validate_metrics(
        groupInfo.metricGroupHandle,
        lzt::metric_streamer_read_data_size(metricStreamerHandle),
        rawData.data());

  } while (metric_helper.running());
  LOG_INFO << "Waiting for process to finish...";
  metric_helper.wait();

  EXPECT_EQ(metric_helper.exit_code(), 0);

  // cleanup
  lzt::metric_streamer_close(metricStreamerHandle);
  lzt::deactivate_metric_groups(device);
  eventPool.destroy_event(eventHandle);
}

TEST(
    zetMetricStreamProcessTest,
    GivenWorkloadExecutingInSeparateProcessWhenStreamingMetricsAndSendInterruptThenExpectValidMetrics) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  // setup monitor
  auto metricGroupInfo = lzt::get_metric_group_info(
      device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

  if (metricGroupInfo.empty()) {
    LOG_INFO << "No metric groups found";
    return;
  }
  // pick a metric group
  auto groupInfo = metricGroupInfo[0];

  LOG_INFO << "Selected metric group: " << groupInfo.metricGroupName
           << " domain: " << groupInfo.domain;

  LOG_INFO << "Activating metric group: " << groupInfo.metricGroupName;
  lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

  ze_event_handle_t eventHandle;
  lzt::zeEventPool eventPool;
  eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                         ZE_EVENT_SCOPE_FLAG_HOST);

  uint32_t notifyEveryNReports = 3000;
  uint32_t samplingPeriod = 1000000;
  zet_metric_streamer_handle_t metricStreamerHandle =
      lzt::metric_streamer_open_for_device(device, groupInfo.metricGroupHandle,
                                           eventHandle, notifyEveryNReports,
                                           samplingPeriod);
  ASSERT_NE(nullptr, metricStreamerHandle);

  //================================================================================
  LOG_INFO << "Starting workload in separate process";
  fs::path helper_path(fs::current_path() / "metrics");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  fs::path helper = bp::search_path("test_metric_helper", paths);
  bp::opstream child_input;
  bp::child metric_helper(helper, "-i", bp::std_in < child_input);

  // start monitor
  LOG_DEBUG << "Waiting for data (event synchronize)...";
  lzt::event_host_synchronize(eventHandle,
                              std::numeric_limits<uint64_t>::max());
  lzt::event_host_reset(eventHandle);

  // read data
  size_t oneReportSize, allReportsSize, numReports;
  oneReportSize = lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
  allReportsSize =
      lzt::metric_streamer_read_data_size(metricStreamerHandle, UINT32_MAX);

  LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
            << ". All reports size:" << allReportsSize;

  EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

  std::vector<uint8_t> rawData;
  lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
  lzt::validate_metrics(
      groupInfo.metricGroupHandle,
      lzt::metric_streamer_read_data_size(metricStreamerHandle),
      rawData.data());

  // send interrupt
  LOG_DEBUG << "Sending interrupt to process";
  child_input << "stop" << std::endl;

  // wait 1 second
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // expect helper has exited
  EXPECT_FALSE(metric_helper.running());

  if (metric_helper.running()) {
    LOG_DEBUG << "Ending Helper";
    metric_helper.terminate();
  }
  LOG_DEBUG << "Process exited";

  // cleanup
  lzt::deactivate_metric_groups(device);
  lzt::metric_streamer_close(metricStreamerHandle);
  eventPool.destroy_event(eventHandle);
}

TEST_F(
    zetMetricStreamerAppendMarkerTest,
    GivenValidMetricGroupWhenTimerBasedStreamerIsCreatedWithAppendStreamerMarkerToImmediateCommandListThenExpectStreamerToSucceed) {

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

    zet_command_list_handle_t commandList =
        lzt::create_immediate_command_list(device);

    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, false);
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo, 1);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ze_event_handle_t eventHandle;
      lzt::zeEventPool eventPool;
      eventPool.create_event(eventHandle, ZE_EVENT_SCOPE_FLAG_HOST,
                             ZE_EVENT_SCOPE_FLAG_HOST);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, eventHandle,
              notifyEveryNReports, samplingPeriod);
      ASSERT_NE(nullptr, metricStreamerHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);

      // Since immediate command list is used, using repeated command list
      // updates to capture metric data
      const uint32_t max_repeat_count = 200;
      for (uint32_t repeat_count = 0; repeat_count < max_repeat_count;
           repeat_count++) {
        zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                        nullptr);

        lzt::append_barrier(commandList);
        uint32_t streamerMarker = 0;
        lzt::commandlist_append_streamer_marker(
            commandList, metricStreamerHandle, ++streamerMarker);
        lzt::append_barrier(commandList);
        lzt::commandlist_append_streamer_marker(
            commandList, metricStreamerHandle, ++streamerMarker);
      }
      ze_result_t eventResult;
      eventResult = zeEventQueryStatus(eventHandle);

      if (ZE_RESULT_SUCCESS == eventResult) {
        size_t oneReportSize, allReportsSize, numReports;
        oneReportSize =
            lzt::metric_streamer_read_data_size(metricStreamerHandle, 1);
        allReportsSize = lzt::metric_streamer_read_data_size(
            metricStreamerHandle, UINT32_MAX);
        LOG_DEBUG << "Event triggered. Single report size: " << oneReportSize
                  << ". All reports size:" << allReportsSize;

        EXPECT_GE(allReportsSize / oneReportSize, notifyEveryNReports);

      } else if (ZE_RESULT_NOT_READY == eventResult) {
        LOG_WARNING << "wait on event returned ZE_RESULT_NOT_READY";
      } else {
        FAIL() << "zeEventQueryStatus() FAILED with " << eventResult;
      }

      std::vector<uint8_t> rawData;
      lzt::metric_streamer_read_data(metricStreamerHandle, &rawData);
      lzt::validate_metrics(
          groupInfo.metricGroupHandle,
          lzt::metric_streamer_read_data_size(metricStreamerHandle),
          rawData.data());

      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      eventPool.destroy_event(eventHandle);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_list(commandList);
  }
}

void run_multi_device_streamer_test(
    const std::vector<ze_device_handle_t> &devices) {

  LOG_INFO << "Testing multi-device metrics streamer";

  if (devices.size() < 2) {
    GTEST_SKIP() << "Skipping test as less than 2 devices are available";
  }

  auto device_0 = devices[0];
  auto device_1 = devices[1];
  auto command_queue_0 = lzt::create_command_queue(device_0);
  auto command_queue_1 = lzt::create_command_queue(device_1);

  auto command_list_0 = lzt::create_command_list(device_0);
  auto command_list_1 = lzt::create_command_list(device_1);

  auto metric_group_info_0 = lzt::get_metric_group_info(
      device_0, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);
  auto metric_group_info_1 = lzt::get_metric_group_info(
      device_1, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true);

  metric_group_info_0 =
      lzt::optimize_metric_group_info_list(metric_group_info_0);
  metric_group_info_1 =
      lzt::optimize_metric_group_info_list(metric_group_info_1);

  auto groupInfo_0 = metric_group_info_0[0];
  auto groupInfo_1 = metric_group_info_1[0];

  lzt::activate_metric_groups(device_0, 1, &groupInfo_0.metricGroupHandle);
  lzt::activate_metric_groups(device_1, 1, &groupInfo_1.metricGroupHandle);

  LOG_INFO << "test metricGroup names " << groupInfo_0.metricGroupName << " "
           << groupInfo_1.metricGroupName;

  ze_event_handle_t eventHandle_0, eventHandle_1;
  lzt::zeEventPool eventPool;
  eventPool.create_event(eventHandle_0, ZE_EVENT_SCOPE_FLAG_HOST,
                         ZE_EVENT_SCOPE_FLAG_HOST);
  eventPool.create_event(eventHandle_1, ZE_EVENT_SCOPE_FLAG_HOST,
                         ZE_EVENT_SCOPE_FLAG_HOST);

  auto notifyEveryNReports = 3000;
  auto samplingPeriod = 1000000;
  auto metricStreamerHandle_0 = lzt::metric_streamer_open_for_device(
      device_0, groupInfo_0.metricGroupHandle, eventHandle_0,
      notifyEveryNReports, samplingPeriod);

  auto metricStreamerHandle_1 = lzt::metric_streamer_open_for_device(
      device_1, groupInfo_1.metricGroupHandle, eventHandle_1,
      notifyEveryNReports, samplingPeriod);

  ASSERT_NE(nullptr, metricStreamerHandle_0);
  ASSERT_NE(nullptr, metricStreamerHandle_1);

  void *a_buffer_0, *b_buffer_0, *c_buffer_0;
  ze_group_count_t tg_0;

  void *a_buffer_1, *b_buffer_1, *c_buffer_1;
  ze_group_count_t tg_1;

  auto function_0 = get_matrix_multiplication_kernel(
      device_0, &tg_0, &a_buffer_0, &b_buffer_0, &c_buffer_0);

  auto function_1 = get_matrix_multiplication_kernel(
      device_1, &tg_1, &a_buffer_1, &b_buffer_1, &c_buffer_1);

  lzt::append_launch_function(command_list_0, function_0, &tg_0, nullptr, 0,
                              nullptr);
  lzt::append_launch_function(command_list_1, function_1, &tg_1, nullptr, 0,
                              nullptr);

  lzt::close_command_list(command_list_0);
  lzt::close_command_list(command_list_1);

  lzt::execute_command_lists(command_queue_0, 1, &command_list_0, nullptr);
  lzt::execute_command_lists(command_queue_1, 1, &command_list_1, nullptr);

  lzt::synchronize(command_queue_0, std::numeric_limits<uint64_t>::max());
  lzt::synchronize(command_queue_1, std::numeric_limits<uint64_t>::max());

  auto eventResult_0 = zeEventQueryStatus(eventHandle_0);
  auto eventResult_1 = zeEventQueryStatus(eventHandle_1);

  if (ZE_RESULT_SUCCESS == eventResult_0 &&
      ZE_RESULT_SUCCESS == eventResult_1) {

  } else {
    LOG_WARNING << "Non-Success zeEventQueryStatus: event_0: " << eventResult_0
                << " event_1: " << eventResult_1;
  }

  std::vector<uint8_t> rawData_0, rawData_1;
  lzt::metric_streamer_read_data(metricStreamerHandle_0, &rawData_0);
  lzt::metric_streamer_read_data(metricStreamerHandle_1, &rawData_1);

  lzt::validate_metrics(
      groupInfo_0.metricGroupHandle,
      lzt::metric_streamer_read_data_size(metricStreamerHandle_0),
      rawData_0.data());
  lzt::validate_metrics(
      groupInfo_1.metricGroupHandle,
      lzt::metric_streamer_read_data_size(metricStreamerHandle_1),
      rawData_1.data());

  // cleanup
  lzt::deactivate_metric_groups(device_0);
  lzt::deactivate_metric_groups(device_1);
  lzt::metric_streamer_close(metricStreamerHandle_0);
  lzt::metric_streamer_close(metricStreamerHandle_1);
  lzt::destroy_function(function_0);
  lzt::destroy_function(function_1);
  lzt::free_memory(a_buffer_0);
  lzt::free_memory(b_buffer_0);
  lzt::free_memory(c_buffer_0);
  lzt::free_memory(a_buffer_1);
  lzt::free_memory(b_buffer_1);
  lzt::free_memory(c_buffer_1);
  lzt::destroy_command_list(command_list_0);
  lzt::destroy_command_list(command_list_1);
  lzt::destroy_command_queue(command_queue_0);
  lzt::destroy_command_queue(command_queue_1);
  eventPool.destroy_event(eventHandle_0);
  eventPool.destroy_event(eventHandle_1);
}

TEST_F(
    zetMetricStreamerTest,
    GivenValidMetricGroupsWhenMultipleDevicesExecutingThenExpectValidMetrics) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_multi_device_streamer_test(devices);
}

TEST_F(
    zetMetricStreamerTest,
    GivenValidMetricGroupsWhenMultipleSubdevicesExecutingThenExpectValidMetrics) {
  auto sub_devices = lzt::get_all_sub_devices();

  run_multi_device_streamer_test(sub_devices);
}
} // namespace
