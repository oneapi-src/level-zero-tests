#include <thread>

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_metric_utils.hpp"

#include "gtest/gtest.h"

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

namespace lzt = level_zero_tests;

namespace {

std::atomic<bool> workloadThreadFlag(false);

void workloadThread(ze_command_queue_handle_t cq, uint32_t numCommandLists,
                    ze_command_list_handle_t *phCommandLists,
                    ze_fence_handle_t hFence) {
  while (workloadThreadFlag) {
    lzt::execute_command_lists(cq, 1, phCommandLists, nullptr);
    lzt::synchronize(cq, std::numeric_limits<uint64_t>::max());
  }
}

class zetMetricsEnableDisableTest : public ::testing::Test {
protected:
  std::vector<ze_device_handle_t> devices;
  ze_device_handle_t device;

  void SetUp() {
    if (!is_ext_supported()) {
      LOG_INFO << "Skipping test as "
                  "ZET_METRICS_RUNTIME_ENABLE_DISABLE_EXP_NAME "
               << "extension is not supported";
      GTEST_SKIP();
    }
    devices = lzt::get_metric_test_device_list();
  }
  bool is_ext_supported() {
    return lzt::check_if_extension_supported(
        lzt::get_default_driver(), ZET_METRICS_RUNTIME_ENABLE_DISABLE_EXP_NAME);
  }
};

class zetMetricEnableDisableStreamerTest : public zetMetricsEnableDisableTest {
protected:
  std::vector<ze_device_handle_t> devices;
  static constexpr uint32_t maxReadAttempts = 20;
  static constexpr uint32_t numberOfReportsReq = 100;
  static constexpr uint32_t timeBeforeReadInNanoSec = 500000000;
  uint32_t samplingPeriod = timeBeforeReadInNanoSec / numberOfReportsReq;
  uint32_t notifyEveryNReports = 9000;
  ze_device_handle_t device;
};

LZT_TEST_F(
    zetMetricsEnableDisableTest,
    GivenMetricsDisabledByEnvironmentWhenMetricsRuntimeNotEnabledThenMetricGroupGetFailsUntilRuntimeEnabled) {

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

    uint32_t metricGroupCount = 0;
    ze_result_t result = zetMetricGroupGet(device, &metricGroupCount, nullptr);
    EXPECT_EQ(result, ZE_RESULT_ERROR_UNINITIALIZED);

    lzt::enable_metrics_runtime(device);
    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true, true);
    EXPECT_GT(metricGroupInfo.size(), 0u) << "No metric groups found";
  }
}

LZT_TEST_F(
    zetMetricsEnableDisableTest,
    GivenMetricsDisabledByEnvironmentWhenMetricsRuntimeEnabledThenMetricGroupGetPassesAfterRuntimeDisabled) {

  for (auto device : devices) {
    lzt::enable_metrics_runtime(device);

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

    lzt::enable_metrics_runtime(device);
    auto metricGroupInfo = lzt::get_metric_group_info(
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true, true);
    EXPECT_GT(metricGroupInfo.size(), 0u) << "No metric groups found";

    lzt::disable_metrics_runtime(device);
    uint32_t metricGroupCount = 0;
    ze_result_t result = zetMetricGroupGet(device, &metricGroupCount, nullptr);
    EXPECT_ZE_RESULT_SUCCESS(result);
  }
}

LZT_TEST_F(
    zetMetricEnableDisableStreamerTest,
    GivenMetricsDisabledByEnvironmentWhenMetricsRuntimeEnabledThenMetricStreamerSucceeds) {

  for (auto device : devices) {
    lzt::enable_metrics_runtime(device);

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
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true, true);
    ASSERT_GT(metricGroupInfo.size(), 0u) << "No metric groups found";
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {
      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer, 8192);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::close_command_list(commandList);

      // Spawn a thread which continuously runs a workload
      workloadThreadFlag = true;
      std::thread thread(workloadThread, commandQueue, 1, &commandList,
                         nullptr);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, nullptr, notifyEveryNReports,
              samplingPeriod);

      // Sleep for timeBeforeReadInNanoSec to ensure required reports are
      // generated
      std::this_thread::sleep_for(
          std::chrono::nanoseconds(timeBeforeReadInNanoSec));
      ASSERT_NE(nullptr, metricStreamerHandle);

      size_t rawDataSize = 0;
      std::vector<uint8_t> rawData;
      rawDataSize = lzt::metric_streamer_read_data_size(metricStreamerHandle,
                                                        notifyEveryNReports);
      EXPECT_GT(rawDataSize, 0);
      rawData.resize(rawDataSize);
      for (uint32_t count = 0; count < maxReadAttempts; count++) {
        lzt::metric_streamer_read_data(
            metricStreamerHandle, notifyEveryNReports, rawDataSize, &rawData);
        if (rawDataSize > 0) {
          break;
        } else {
          std::this_thread::sleep_for(std::chrono::nanoseconds(samplingPeriod));
        }
      }

      LOG_INFO << "rawDataSize " << rawDataSize;
      // Stop the worker thread running the workload
      workloadThreadFlag = false;
      thread.join();

      lzt::validate_metrics(groupInfo.metricGroupHandle, rawDataSize,
                            rawData.data());
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      lzt::reset_command_list(commandList);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
    EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
  }
}

LZT_TEST_F(
    zetMetricEnableDisableStreamerTest,
    GivenMetricsDisabledByEnvironmentWhenMetricGroupisActivatedThenMetricsRuntimeDisableFailsUntilMetricGroupIsDeactivated) {

  for (auto device : devices) {
    lzt::enable_metrics_runtime(device);
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
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED, true, true);
    ASSERT_GT(metricGroupInfo.size(), 0u) << "No metric groups found";
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {
      lzt::enable_metrics_runtime(device);
      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);

      ASSERT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer, 8192);
      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);
      lzt::close_command_list(commandList);

      // Spawn a thread which continuously runs a workload
      workloadThreadFlag = true;
      std::thread thread(workloadThread, commandQueue, 1, &commandList,
                         nullptr);

      zet_metric_streamer_handle_t metricStreamerHandle =
          lzt::metric_streamer_open_for_device(
              device, groupInfo.metricGroupHandle, nullptr, notifyEveryNReports,
              samplingPeriod);

      EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);

      // Sleep for timeBeforeReadInNanoSec to ensure required reports are
      // generated
      std::this_thread::sleep_for(
          std::chrono::nanoseconds(timeBeforeReadInNanoSec));
      ASSERT_NE(nullptr, metricStreamerHandle);

      size_t rawDataSize = 0;
      std::vector<uint8_t> rawData;
      rawDataSize = lzt::metric_streamer_read_data_size(metricStreamerHandle,
                                                        notifyEveryNReports);
      EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);
      EXPECT_GT(rawDataSize, 0);
      rawData.resize(rawDataSize);
      for (uint32_t count = 0; count < maxReadAttempts; count++) {
        lzt::metric_streamer_read_data(
            metricStreamerHandle, notifyEveryNReports, rawDataSize, &rawData);
        if (rawDataSize > 0) {
          std::vector<zet_typed_value_t> metricValues;
          std::vector<uint32_t> metricValueSets;
          ze_result_t result =
              lzt::metric_calculate_metric_values_from_raw_data(
                  groupInfo.metricGroupHandle, rawData, metricValues,
                  metricValueSets);
          ASSERT_ZE_RESULT_SUCCESS(result);
          EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                    ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);
          break;
        } else {
          std::this_thread::sleep_for(std::chrono::nanoseconds(samplingPeriod));
        }
      }

      LOG_INFO << "rawDataSize " << rawDataSize;
      // Stop the worker thread running the workload
      workloadThreadFlag = false;
      thread.join();

      lzt::validate_metrics(groupInfo.metricGroupHandle, rawDataSize,
                            rawData.data());
      lzt::metric_streamer_close(metricStreamerHandle);
      lzt::deactivate_metric_groups(device);
      EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
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

using zetMetricsEnableDisableQueryTest = zetMetricsEnableDisableTest;

LZT_TEST_F(
    zetMetricsEnableDisableQueryTest,
    GivenMetricsDisabledByEnvironmentWhenMetricsRuntimeAlsoEnabledThenMetricQuerySucceeds) {

  for (auto device : devices) {
    lzt::enable_metrics_runtime(device);
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
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false, true);
    ASSERT_GT(metricGroupInfo.size(), 0u) << "No query metric groups found";
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {

      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      zet_metric_query_pool_handle_t metric_query_pool_handle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);
      ASSERT_NE(nullptr, metric_query_pool_handle)
          << "failed to create metric query pool handle";
      zet_metric_query_handle_t metric_query_handle =
          lzt::metric_query_create(metric_query_pool_handle);
      ASSERT_NE(nullptr, metric_query_handle)
          << "failed to create metric query handle";

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);
      lzt::append_metric_query_begin(commandList, metric_query_handle);
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
      lzt::append_metric_query_end(commandList, metric_query_handle,
                                   eventHandle);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(eventHandle));
      std::vector<uint8_t> rawData;

      lzt::metric_query_get_data(metric_query_handle, &rawData);

      eventPool.destroy_event(eventHandle);
      lzt::destroy_metric_query(metric_query_handle);
      lzt::destroy_metric_query_pool(metric_query_pool_handle);

      lzt::deactivate_metric_groups(device);
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);

      lzt::reset_command_list(commandList);
    }

    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
    EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
  }
}

LZT_TEST_F(
    zetMetricsEnableDisableQueryTest,
    GivenMetricsDisabledByEnvironmentWhenMetricsRuntimeAlsoEnabledThenRuntimeDisableFailsUntilMetricGroupIsDeactivated) {

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
        device, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED, false, true);
    ASSERT_GT(metricGroupInfo.size(), 0u) << "No query metric groups found";
    metricGroupInfo = lzt::optimize_metric_group_info_list(metricGroupInfo);

    for (auto groupInfo : metricGroupInfo) {
      lzt::enable_metrics_runtime(device);
      LOG_INFO << "test metricGroup name " << groupInfo.metricGroupName;
      zet_metric_query_pool_handle_t metric_query_pool_handle =
          lzt::create_metric_query_pool_for_device(
              device, 1000, ZET_METRIC_QUERY_POOL_TYPE_PERFORMANCE,
              groupInfo.metricGroupHandle);
      ASSERT_NE(nullptr, metric_query_pool_handle)
          << "failed to create metric query pool handle";
      zet_metric_query_handle_t metric_query_handle =
          lzt::metric_query_create(metric_query_pool_handle);
      ASSERT_NE(nullptr, metric_query_handle)
          << "failed to create metric query handle";

      lzt::activate_metric_groups(device, 1, &groupInfo.metricGroupHandle);
      EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);
      lzt::append_metric_query_begin(commandList, metric_query_handle);
      EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);
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
      lzt::append_metric_query_end(commandList, metric_query_handle,
                                   eventHandle);
      EXPECT_EQ(zetDeviceDisableMetricsExp(device),
                ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE);

      lzt::close_command_list(commandList);
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      lzt::synchronize(commandQueue, UINT64_MAX);

      EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(eventHandle));
      std::vector<uint8_t> rawData;

      lzt::metric_query_get_data(metric_query_handle, &rawData);

      eventPool.destroy_event(eventHandle);
      lzt::destroy_metric_query(metric_query_handle);
      lzt::destroy_metric_query_pool(metric_query_pool_handle);

      lzt::deactivate_metric_groups(device);
      EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
      lzt::destroy_function(function);
      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);

      lzt::reset_command_list(commandList);
    }

    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
    EXPECT_ZE_RESULT_SUCCESS(zetDeviceDisableMetricsExp(device));
  }
}

} // namespace