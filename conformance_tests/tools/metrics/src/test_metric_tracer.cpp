/*
 *
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>
#include <numeric>

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

using lzt::to_nanoseconds;

using lzt::to_u32;
using lzt::to_u8;

class zetMetricTracerTest : public ::testing::Test {
protected:
  std::vector<lzt::activatable_metric_group_handle_list_for_device_t>
      tracer_supporting_devices_list;

  void SetUp() override {
    driver = lzt::get_default_driver();
    device = lzt::get_default_device(driver);
    context = lzt::create_context(driver);
    devices = lzt::get_metric_test_device_list();

    lzt::generate_device_list_with_activatable_metric_group_handles(
        devices, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED,
        tracer_supporting_devices_list);

    if (tracer_supporting_devices_list.size() == 0) {
      GTEST_SKIP()
          << "no devices that have metric groups of type tracer were found";
    }
  }

  void TearDown() override { lzt::destroy_context(context); }

  ze_device_handle_t device;
  ze_driver_handle_t driver;
  ze_context_handle_t context;

  std::vector<ze_device_handle_t> devices;

  zet_metric_tracer_exp_desc_t tracer_descriptor = {
      ZET_STRUCTURE_TYPE_TRACER_EXP_DESC, nullptr, 10};

  zet_metric_tracer_exp_handle_t tracer_handle;
};

LZT_TEST_F(
    zetMetricTracerTest,
    GivenActivatedMetricGroupsWhenTracerIsCreatedWithOneOrMultipleMetricsGroupsThenTracerCreateSucceeds) {

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0U);
    for (uint32_t i = 0U; i < num_grp_handles; i++) {
      lzt::activate_metric_groups(device, i + 1, grp_handles.data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1, grp_handles.data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle);
      EXPECT_ZE_RESULT_SUCCESS(result);

      zet_metric_tracer_exp_handle_t metric_tracer_handle1;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1, grp_handles.data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle1);
      EXPECT_NE(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp for the second time failed with "
             "error code "
          << result;

      lzt::metric_tracer_destroy(metric_tracer_handle);
      lzt::deactivate_metric_groups(device);
    }
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    WhenCreatingTracerIfNotificationEventHandleIsValidAndNotifyEveryNBytesIsZeroThenNotifyEveryNBytesIsSetToNearestPossibleValue) {
  /* After the tracer notification event, notifyEveryNBytes will be set to a
   * valid, hardware-supported, non-zero value. */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_event_pool_handle_t notification_event_pool;
    notification_event_pool = lzt::create_event_pool(
        lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    ze_event_handle_t notification_event;
    ze_event_desc_t notification_event_descriptor = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST,
        ZE_EVENT_SCOPE_FLAG_DEVICE};
    notification_event = lzt::create_event(notification_event_pool,
                                           notification_event_descriptor);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    tracer_descriptor.notifyEveryNBytes = 0;
    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, notification_event, &metric_tracer_handle);
    EXPECT_NE(0, tracer_descriptor.notifyEveryNBytes);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::destroy_event(notification_event);
    lzt::destroy_event_pool(notification_event_pool);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    WhenCreatingTracerIfNotificationEventHandleIsValidAndNotifyEveryNBytesIsUnsignedIntMaxThenNotifyEveryNBytesIsSetToNearestPossibleValue) {
  /* After the tracer notification event, notifyEveryNBytes will be set to a
   * valid, hardware-supported, non-zero value, but not to an excessively large
   * value like UINT_MAX */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_event_pool_handle_t notification_event_pool;
    notification_event_pool = lzt::create_event_pool(
        lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    ze_event_handle_t notification_event;
    ze_event_desc_t notification_event_descriptor = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST,
        ZE_EVENT_SCOPE_FLAG_DEVICE};
    notification_event = lzt::create_event(notification_event_pool,
                                           notification_event_descriptor);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    tracer_descriptor.notifyEveryNBytes = UINT32_MAX;
    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, notification_event, &metric_tracer_handle);
    EXPECT_NE(UINT32_MAX, tracer_descriptor.notifyEveryNBytes);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::destroy_event(notification_event);
    lzt::destroy_event_pool(notification_event_pool);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenNotificationEventHasHappenedThenEnsureAvailableRawDataSizeIsMoreThanNotifyEveryNBytes) {
  /* While executing a workload, right after the tracer notification event, raw
   * data greater than or equal to notifyEveryNBytes will be available */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);
    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 64);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);
    ze_event_pool_handle_t notification_event_pool;
    notification_event_pool = lzt::create_event_pool(
        lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    ze_event_handle_t notification_event;
    ze_event_desc_t notification_event_descriptor = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST,
        ZE_EVENT_SCOPE_FLAG_DEVICE};
    notification_event = lzt::create_event(notification_event_pool,
                                           notification_event_descriptor);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    tracer_descriptor.notifyEveryNBytes = 8192;
    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, notification_event, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
    LOG_DEBUG << "synchronize on the tracer notification event";
    lzt::event_host_synchronize(notification_event,
                                std::numeric_limits<uint64_t>::max());
    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, raw_data.data()));
    EXPECT_GE(raw_data_size, tracer_descriptor.notifyEveryNBytes)
        << "raw data available should be greater than equal to "
           "notifyEveryNBytes";
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::destroy_event(notification_event);
    lzt::destroy_event_pool(notification_event_pool);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedAndEnabledThenTracerCannotBeDestroyedWithoutFirstBeingDisabled) {
  /* A tracer still enabled cannot be destroyed */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;
    lzt::display_device_properties(device);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());
    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "destroy tracer";
    result = zetMetricTracerDestroyExp(metric_tracer_handle);
    EXPECT_EQ(ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE, result);
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    result = zetMetricTracerDestroyExp(metric_tracer_handle);
    EXPECT_ZE_RESULT_SUCCESS(result);
    lzt::deactivate_metric_groups(device);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWhenTracerIsNotEnabledThenExpectNoRawDataToBeAvailable) {
  /* When the tracer is not enabled, no raw data is available to be read */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);

    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());

    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    EXPECT_EQ(ZE_RESULT_NOT_READY,
              zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                         raw_data.data()))
        << "tracer is not enabled, zetMetricTracerReadDataExp should return "
           "ZE_RESULT_NOT_READY";
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsEnabledWhenQueryingForDataSizeWithZeroSizeThenExpectInvalidArgumentError) {
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);

    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());

    size_t raw_data_size = 0;
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
              zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                         nullptr))
        << "zetMetricTracerReadDataExp with raw_data_size = 0 should return "
           "ZE_RESULT_ERROR_INVALID_ARGUMENT";

    lzt::metric_tracer_disable(metric_tracer_handle, true);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWhenTracerIsAsynchronouslyEnabledThenExpectValidRawDataToBeAvailableOnceTracerIsEnabled) {
  /* When the tracer is asynchronously enabled, wait for the tracer to be
   * enabled. Once enabled, zetMetricTracerReadDataExp returns ZE_RESULT_SUCCESS
   * and valid raw data is available */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, false);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
    const char *value_string =
        std::getenv("LZT_METRIC_ENABLE_TRACER_MAX_WAIT_TIME");
    uint32_t max_wait_time_in_milliseconds = 10;
    if (value_string != nullptr) {
      uint32_t value = to_u32(value_string);
      max_wait_time_in_milliseconds =
          value != 0 ? value : max_wait_time_in_milliseconds;
      max_wait_time_in_milliseconds =
          std::min(max_wait_time_in_milliseconds, 100u);
    }

    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    /* wait for the tracer to get enabled */
    do {
      raw_data_size = raw_data.size();
      result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                          raw_data.data());
      if (result == ZE_RESULT_NOT_READY) {
        LOG_INFO
            << "Waiting for tracer to be enabled. zetMetricTracerReadDataExp "
               "will be called again after sleep";
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        max_wait_time_in_milliseconds--;
      }
      if (max_wait_time_in_milliseconds == 0) {
        FAIL() << "failed while trying to enable tracer asynchronously, waited "
                  "for "
               << max_wait_time_in_milliseconds << " ms";
      }
    } while (result == ZE_RESULT_NOT_READY);
    EXPECT_ZE_RESULT_SUCCESS(result);
    EXPECT_NE(0, raw_data_size) << "zetMetricTracerReadDataExp reports that "
                                   "there are no metrics available to read";
    uint64_t raw_data_accumulate = std::accumulate(
        raw_data.begin(), raw_data.begin() + raw_data_size, 0ULL);
    EXPECT_NE(0, raw_data_accumulate)
        << "all raw data entries are zero, zetMetricTracerReadDataExp is "
           "expected to read useful data";
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsEnabledWhenTracerIsAsynchronouslyDisabledThenNoFurtherRawDataIsAvailableToReadOnceTracerIsDisabled) {
  /* If the tracer is asynchronously disabled, the test repeatedly calls
   * zetMetricTracerReadDataExp until it returns ZE_RESULT_NOT_READY, confirming
   * that the tracer has been fully disabled and no further data is available to
   * read */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());

    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    uint64_t raw_data_accumulate{};
    ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, raw_data.data()));
    ASSERT_NE(0, raw_data_size) << "zetMetricTracerReadDataExp reports that "
                                   "there are no metrics available to read";

    raw_data_accumulate = std::accumulate(
        raw_data.begin(), raw_data.begin() + raw_data_size, 0ULL);
    ASSERT_NE(0, raw_data_accumulate)
        << "all raw data entries are zero, zetMetricTracerReadDataExp is "
           "expected to read useful data";

    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());

    lzt::metric_tracer_disable(metric_tracer_handle, false);

    const char *value_string =
        std::getenv("LZT_METRIC_DISABLE_TRACER_MAX_WAIT_TIME");
    uint32_t max_wait_time_in_milliseconds = 10;
    if (value_string != nullptr) {
      uint32_t value = to_u32(value_string);
      max_wait_time_in_milliseconds =
          value != 0 ? value : max_wait_time_in_milliseconds;
      max_wait_time_in_milliseconds =
          std::min(max_wait_time_in_milliseconds, 100u);
    }
    /* read raw data while API returns ZE_RESULT_SUCCESS and
     * ZE_RESULT_NOT_READY(for the last piece of data) */
    do {
      raw_data_size = raw_data.size();
      result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                          raw_data.data());
      EXPECT_TRUE(ZE_RESULT_SUCCESS == result || ZE_RESULT_NOT_READY == result)
          << "zetMetricTracerReadDataExp failed while retrieving the raw data";

      if (result == ZE_RESULT_SUCCESS && raw_data_size != 0) {
        raw_data_accumulate = std::accumulate(
            raw_data.begin(), raw_data.begin() + raw_data_size, 0ULL);
        EXPECT_NE(0, raw_data_accumulate)
            << "all raw data entries are zero, zetMetricTracerReadDataExp is "
               "expected to read useful data";
      }
      if (result == ZE_RESULT_SUCCESS) {
        LOG_DEBUG
            << "waiting for tracer to be disabled. zetMetricTracerReadDataExp "
               "will be called again after sleep";
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        max_wait_time_in_milliseconds--;
      }
      if (max_wait_time_in_milliseconds == 0) {
        FAIL() << "failed while trying to disable tracer asynchronously, "
                  "waited for "
               << max_wait_time_in_milliseconds << " ms";
      }
    } while (result == ZE_RESULT_SUCCESS);
    raw_data_size = raw_data.size();
    result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                        raw_data.data());
    EXPECT_EQ(ZE_RESULT_NOT_READY, result);

    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsEnabledWhenRequestingMoreRawDataThanAvailableThenReturnOnlyWhatIsAvailable) {
  /* When allocating a larger buffer than needed, ensure that writes happen only
   * for the size of the raw data available */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;
    lzt::display_device_properties(device);
    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());
    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
    lzt::metric_tracer_disable(metric_tracer_handle, true);

    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> temp_buffer(raw_data_size, 0);
    EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, temp_buffer.data()));
    EXPECT_NE(0, raw_data_size)
        << "zetMetricTracerReadDataExp reports that there are no "
           "metrics available to read";
    if (raw_data_size != 0) {
      const size_t extra_buffer_size = 24;
      const uint8_t raw_data_init_val = 0xBE;
      raw_data_size += extra_buffer_size;

      std::vector<uint8_t> raw_data(raw_data_size, raw_data_init_val);
      EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
          metric_tracer_handle, &raw_data_size, raw_data.data()));

      uint64_t excess_buffer_raw_data_sum = std::accumulate(
          raw_data.end() - extra_buffer_size, raw_data.end(), 0ULL);
      EXPECT_EQ((raw_data_init_val * extra_buffer_size),
                excess_buffer_raw_data_sum)
          << "zetMetricTracerReadDataExp should not return more raw data than "
             "what is available";
    }
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWithOneOrMoreActivatedMetricGroupWhenTracerIsEnabledSynchronouslyThenExpectTracerEnableToSucceed) {

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0U);
    for (uint32_t i = 0U; i < num_grp_handles; i++) {
      lzt::activate_metric_groups(device, i + 1, grp_handles.data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle0;
      lzt::metric_tracer_create(lzt::get_default_context(), device, i + 1,
                                grp_handles.data(), &tracer_descriptor, nullptr,
                                &metric_tracer_handle0);

      lzt::metric_tracer_enable(metric_tracer_handle0, true);

      result = zetMetricTracerDestroyExp(metric_tracer_handle0);
      ASSERT_NE(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp succeeded on a tracer that is "
             "still enabled";

      lzt::metric_tracer_disable(metric_tracer_handle0, true);
      lzt::metric_tracer_destroy(metric_tracer_handle0);
      lzt::deactivate_metric_groups(device);
    }
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWithOneOrMoreActivatedMetricGroupWhenTracerIsEnabledAndDisabledAsynchronouslyThenExpectTracerEnableAndDisableToSucceed) {

  constexpr int32_t number_of_retries = 5;
  constexpr int32_t retry_wait_milliseconds = 5;
  ze_result_t result;

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0U);
    for (uint32_t i = 0U; i < num_grp_handles; i++) {
      lzt::activate_metric_groups(device, i + 1, grp_handles.data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      lzt::metric_tracer_create(lzt::get_default_context(), device, i + 1,
                                grp_handles.data(), &tracer_descriptor, nullptr,
                                &metric_tracer_handle);

      lzt::metric_tracer_enable(metric_tracer_handle, false);

      int32_t j = 0;
      size_t initial_buffer_size = 1024 * 1024; /* 1MB buffer */
      std::vector<uint8_t> initial_buffer(initial_buffer_size, 0);
      do {
        size_t raw_data_size = initial_buffer.size();
        result = zetMetricTracerReadDataExp(
            metric_tracer_handle, &raw_data_size, initial_buffer.data());
        if (result == ZE_RESULT_NOT_READY) {
          if (j == number_of_retries) {
            FAIL() << "Exceeded limit of retries of "
                      "zetMetricTracerReadDataExp "
                      "waiting for "
                      "the tracer to be enabled";
            break;
          }

          LOG_INFO << "zetMetricTracerReadDataExp will be retried, waiting for "
                      "tracer handle to "
                      "be enabled. sleeping and doing retry number "
                   << j;
          std::this_thread::sleep_for(
              std::chrono::milliseconds(retry_wait_milliseconds));
        }
        j++;
      } while (result == ZE_RESULT_NOT_READY);

      ASSERT_ZE_RESULT_SUCCESS(result);

      lzt::metric_tracer_disable(metric_tracer_handle, false);

      int32_t k = 0;
      size_t disable_buffer_size = 1024 * 1024; /* 1MB buffer */
      std::vector<uint8_t> raw_data_buffer(disable_buffer_size, 0);
      do {
        size_t raw_data_size = raw_data_buffer.size();
        result = zetMetricTracerReadDataExp(
            metric_tracer_handle, &raw_data_size, raw_data_buffer.data());
        if (result == ZE_RESULT_SUCCESS && raw_data_size != 0) {
          if (k == number_of_retries) {
            FAIL() << "Exceeded limit of retries of "
                      "zetMetricTracerReadDataExp "
                      "waiting for "
                      "the tracer to be disabled rawDataSize "
                   << raw_data_size;
            break;
          }

          LOG_DEBUG << "zetMetricTracerReadDataExp rawDataSize "
                    << raw_data_size
                    << " will be retried, waiting for "
                       "tracer handle to "
                       "be disabled. sleeping and doing retry number "
                    << k;
          std::this_thread::sleep_for(
              std::chrono::milliseconds(retry_wait_milliseconds));
        }
        k++;
      } while (result == ZE_RESULT_SUCCESS);

      ASSERT_EQ(result, ZE_RESULT_NOT_READY)
          << "zetMetricTracerReadDataExp() called on a disabled tracer did not "
             "transition to the disabled state";

      lzt::metric_tracer_destroy(metric_tracer_handle);

      lzt::deactivate_metric_groups(device);
    }
  }
}

static void
executeMatrixMultiplyWorkload(ze_device_handle_t device,
                              ze_command_queue_handle_t &commandQueue,
                              zet_command_list_handle_t &commandList) {
  void *a_buffer, *b_buffer, *c_buffer;
  ze_group_count_t tg;
  ze_kernel_handle_t function = get_matrix_multiplication_kernel(
      device, &tg, &a_buffer, &b_buffer, &c_buffer);

  zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                  nullptr);

  lzt::close_command_list(commandList);
  lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
  lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);
  lzt::reset_command_list(commandList);
}

void run_metric_tracer_read_test(
    ze_device_handle_t &device,
    std::vector<lzt::activatable_metric_group_handle_list_for_device_t>
        &tracer_supporting_devices_list,
    zet_metric_tracer_exp_desc_t &tracer_descriptor, bool synchronous) {
  constexpr int32_t number_of_retries = 5;
  constexpr int32_t retry_wait_milliseconds = 5;
  ze_result_t result;

  std::string test_mode = synchronous ? "Synchronous" : "Asynchronous";

  LOG_INFO << "testing zetMetricTracerReadDataExp with " << test_mode
           << " tracer Enable and Disable";

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0U);
    for (uint32_t i = 0U; i < num_grp_handles; i++) {
      lzt::activate_metric_groups(device, i + 1, grp_handles.data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      lzt::metric_tracer_create(lzt::get_default_context(), device, i + 1,
                                grp_handles.data(), &tracer_descriptor, nullptr,
                                &metric_tracer_handle);

      lzt::metric_tracer_enable(metric_tracer_handle, synchronous);

      if (!synchronous) {
        int32_t j = 0;
        size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
        std::vector<uint8_t> raw_data_buffer(raw_data_size, 0);
        do {
          raw_data_size = raw_data_buffer.size();
          result = zetMetricTracerReadDataExp(
              metric_tracer_handle, &raw_data_size, raw_data_buffer.data());
          if (result == ZE_RESULT_NOT_READY) {
            if (j == number_of_retries) {
              FAIL() << "Exceeded limit of retries of "
                        "zetMetricTracerReadDataExp "
                        "waiting for "
                        "the tracer to be enabled";
              break;
            }

            LOG_INFO << "zetMetricTracerReadDataExp will be retried, "
                        "waiting for "
                        "tracer handle to "
                        "be enabled. sleeping and doing retry number "
                     << j;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(retry_wait_milliseconds));
          }
          j++;
        } while (result == ZE_RESULT_NOT_READY);

        ASSERT_ZE_RESULT_SUCCESS(result);
      }

      executeMatrixMultiplyWorkload(device, commandQueue, commandList);

      /* Read partial data while tracer is enabled */
      size_t enabled_read_data_size = 1024; /* 1KB buffer */
      std::vector<uint8_t> enabled_raw_data(enabled_read_data_size);

      ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
          metric_tracer_handle, &enabled_read_data_size,
          enabled_raw_data.data()));
      ASSERT_NE(enabled_read_data_size, 0)
          << "zetMetricTracerReadDataExp on an enabled "
             "tracer returned zero data size";

      lzt::metric_tracer_disable(metric_tracer_handle, synchronous);

      /* Read remaining data after disable */
      size_t disabled_read_data_size = 1024 * 1024; /* 1MB buffer */
      std::vector<uint8_t> disabled_raw_data(disabled_read_data_size);

      ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
          metric_tracer_handle, &disabled_read_data_size,
          disabled_raw_data.data()));
      ASSERT_NE(disabled_read_data_size, 0)
          << "zetMetricTracerReadDataExp with "
             "non-null data buffer and disabled "
             "tracer has returned no data";
      if (!synchronous) {
        int32_t k = 0;
        size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
        std::vector<uint8_t> final_data_buffer(raw_data_size, 0);
        do {
          raw_data_size = final_data_buffer.size();
          result = zetMetricTracerReadDataExp(
              metric_tracer_handle, &raw_data_size, final_data_buffer.data());
          if (result == ZE_RESULT_SUCCESS) {
            if (k == number_of_retries) {
              FAIL() << "Exceeded limit of retries of "
                        "zetMetricTracerReadDataExp "
                        "waiting for "
                        "the tracer to be disabled";
              break;
            }

            LOG_INFO << "zetMetricTracerReadDataExp will be retried, "
                        "waiting for "
                        "tracer handle to "
                        "be disabled. sleeping and doing retry number "
                     << k;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(retry_wait_milliseconds));
          }
          k++;
        } while (result == ZE_RESULT_SUCCESS);
      }

      lzt::metric_tracer_destroy(metric_tracer_handle);
      lzt::deactivate_metric_groups(device);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenSynchronouslyEnabledAndDisabledTracerThenExpectTracerReadsToSucceed) {
  run_metric_tracer_read_test(device, tracer_supporting_devices_list,
                              tracer_descriptor, true);
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenAsynchronouslyEnabledAndDisabledTracerThenExpectTracerReadsToSucceed) {
  run_metric_tracer_read_test(device, tracer_supporting_devices_list,
                              tracer_descriptor, false);
}

LZT_TEST_F(zetMetricTracerTest,
           GivenTracerIsCreatedThenDecoderCreateAndDestroySucceed) {

  ze_result_t result;

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle = nullptr;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);

    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);
    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
  }
}

LZT_TEST_F(zetMetricTracerTest,
           GivenDecoderIsCreatedThenGetDecodableMetricsSucceeds) {

  ze_result_t result;

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle = nullptr;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);

    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);

    uint32_t num_decodable_metrics = 0;
    ASSERT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
        metric_decoder_handle, &num_decodable_metrics, nullptr));
    ASSERT_GT(num_decodable_metrics, 0u)
        << "zetMetricDecoderGetDecodableMetricsExp reports that there are no "
           "decodable metrics";

    uint32_t new_num_decodable_metrics = num_decodable_metrics + 1;
    std::vector<zet_metric_handle_t> metric_handles(new_num_decodable_metrics);
    ASSERT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
        metric_decoder_handle, &new_num_decodable_metrics,
        metric_handles.data()));
    ASSERT_EQ(new_num_decodable_metrics, num_decodable_metrics)
        << "zetMetricDecoderGetDecodableMetricsExp reported an unexpected "
           "value "
           "for the count of decodable metrics";

    ASSERT_GT(num_decodable_metrics, 0U);
    for (uint32_t i = 0U; i < num_decodable_metrics; i++) {
      bool valid_type;
      zet_metric_properties_t metric_properties;
      lzt::get_metric_properties(metric_handles[i], &metric_properties);
      size_t metric_name_string_length;
      metric_name_string_length =
          strnlen(metric_properties.name, ZET_MAX_METRIC_NAME);
      ASSERT_GT(metric_name_string_length, 0)
          << "the name string for this metric is of zero length";
      ASSERT_LT(metric_name_string_length, ZET_MAX_METRIC_NAME)
          << "the name string for this metric is not null terminated";
      valid_type = lzt::verify_value_type(metric_properties.resultType);
      EXPECT_TRUE(valid_type)
          << "metric properties for metric " << metric_properties.name
          << " has an incorrect resultType field "
          << metric_properties.resultType;
      valid_type = lzt::verify_metric_type(metric_properties.metricType);
      EXPECT_TRUE(valid_type)
          << "metric properties for metric " << metric_properties.name
          << " has an invalid metricType field "
          << metric_properties.metricType;
    }

    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenDecoderIsCreatedAndWorkloadExecutedAndTracerIsEnabledThenDecoderSucceedsDecodingMetrics) {

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0U);
    for (uint32_t i = 0U; i < num_grp_handles; i++) {
      LOG_DEBUG << "Number of metric groups being tested is " << i + 1;

      ze_event_pool_handle_t notification_event_pool;
      notification_event_pool = lzt::create_event_pool(
          lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);

      ze_event_handle_t notification_event;
      ze_event_desc_t notification_event_descriptor = {
          ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST,
          ZE_EVENT_SCOPE_FLAG_DEVICE};
      notification_event = lzt::create_event(notification_event_pool,
                                             notification_event_descriptor);

      lzt::activate_metric_groups(device, i + 1, grp_handles.data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      lzt::metric_tracer_create(lzt::get_default_context(), device, i + 1,
                                grp_handles.data(), &tracer_descriptor,
                                notification_event, &metric_tracer_handle);

      lzt::metric_tracer_enable(metric_tracer_handle, true);

      LOG_DEBUG << "create tracer decoder";
      zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
      lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);

      uint32_t decodable_metric_count =
          lzt::metric_decoder_get_decodable_metrics_count(
              metric_decoder_handle);

      std::vector<zet_metric_handle_t> decodable_metric_handles(
          decodable_metric_count);
      lzt::metric_decoder_get_decodable_metrics(metric_decoder_handle,
                                                &decodable_metric_handles);
      decodable_metric_count = to_u32(decodable_metric_handles.size());
      LOG_DEBUG << "found " << decodable_metric_count << " decodable metrics";

      void *a_buffer, *b_buffer, *c_buffer;
      ze_group_count_t tg;
      ze_kernel_handle_t function = get_matrix_multiplication_kernel(
          device, &tg, &a_buffer, &b_buffer, &c_buffer);

      zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                      nullptr);

      lzt::close_command_list(commandList);

      LOG_DEBUG << "execute workload";
      lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

      LOG_DEBUG << "synchronize on the tracer notification event";
      std::chrono::steady_clock::time_point startTime =
          std::chrono::steady_clock::now();

      uint64_t event_wait_time_ns = 50000000000; /* 50 seconds */
      result = zeEventHostSynchronize(notification_event, event_wait_time_ns);

      std::chrono::steady_clock::time_point endTime =
          std::chrono::steady_clock::now();

      auto elapsedTime = to_nanoseconds(endTime - startTime);
      LOG_DEBUG << "resumed after synchronizing on the tracer notification "
                   "event, blocked for "
                << elapsedTime << " nanoseconds";

      if (result != ZE_RESULT_SUCCESS) {
        LOG_WARNING << "zeEventHostSynchronize on notification event waiting "
                       "for trace data failed with error code "
                    << result;
      }

      /* read data */
      size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
      std::vector<uint8_t> raw_data(raw_data_size, 0);
      ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
          metric_tracer_handle, &raw_data_size, raw_data.data()));
      ASSERT_NE(raw_data_size, 0)
          << "zetMetricTracerReadDataExp returned no data";
      raw_data.resize(raw_data_size);

      /* decode data */
      uint32_t metric_entry_count = 0;
      uint32_t set_count = 0;
      lzt::metric_tracer_decode_get_various_counts(
          metric_decoder_handle, &raw_data_size, &raw_data,
          decodable_metric_count, &decodable_metric_handles, &set_count,
          &metric_entry_count);
      EXPECT_NE(0u, metric_entry_count) << "zetMetricTracerDecodeExp reports "
                                           "that there are no metric entries "
                                           "to be decoded";
      std::vector<uint32_t> metric_entries_per_set_count(set_count);
      std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count);

      lzt::metric_tracer_decode(
          metric_decoder_handle, &raw_data_size, &raw_data,
          decodable_metric_count, &decodable_metric_handles, &set_count,
          &metric_entries_per_set_count, &metric_entry_count, &metric_entries);

      LOG_DEBUG << "Actual Decoded Metric Entry Count: " << metric_entry_count
                << "\n";
      LOG_DEBUG << "Raw data decoded: " << raw_data_size << " bytes"
                << std::endl;

      uint32_t set_entry_start = 0;
      for (uint32_t set_index = 0; set_index < set_count; set_index++) {
        LOG_DEBUG << "Set number: " << set_index << " Entries in set: "
                  << metric_entries_per_set_count[set_index] << "\n";

        for (uint32_t index = set_entry_start;
             index < set_entry_start + metric_entries_per_set_count[set_index];
             index++) {
          auto &metric_entry = metric_entries[index];
          zet_metric_properties_t metric_properties = {};
          lzt::get_metric_properties(
              decodable_metric_handles[metric_entry.metricIndex],
              &metric_properties);
          LOG_DEBUG << "Component: " << metric_properties.component
                    << " Decodable metric name: " << metric_properties.name;
          switch (metric_properties.resultType) {
          case ZET_VALUE_TYPE_UINT32:
          case ZET_VALUE_TYPE_UINT8:
          case ZET_VALUE_TYPE_UINT16:
            LOG_DEBUG << "\t value: " << metric_entry.value.ui32;
            break;
          case ZET_VALUE_TYPE_UINT64:
            LOG_DEBUG << "\t value: " << metric_entry.value.ui64;
            break;
          case ZET_VALUE_TYPE_FLOAT32:
            LOG_DEBUG << "\t value: " << metric_entry.value.fp32;
            break;
          case ZET_VALUE_TYPE_FLOAT64:
            LOG_DEBUG << "\t value: " << metric_entry.value.fp64;
            break;
          case ZET_VALUE_TYPE_BOOL8:
            LOG_DEBUG << "\t value: "
                      << (metric_entry.value.b8 ? "TRUE" : "FALSE");
            break;
          default:
            LOG_ERROR << "Encountered unsupported Type "
                      << metric_properties.resultType;
            break;
          }
          LOG_DEBUG << "timestamp " << metric_entry.timeStamp;
        }

        set_entry_start += metric_entries_per_set_count[set_index];
      }

      LOG_DEBUG << "synchronize with completion of workload";
      lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());

      lzt::free_memory(a_buffer);
      lzt::free_memory(b_buffer);
      lzt::free_memory(c_buffer);
      lzt::reset_command_list(commandList);

      lzt::metric_decoder_destroy(metric_decoder_handle);
      lzt::metric_tracer_disable(metric_tracer_handle, true);
      lzt::metric_tracer_destroy(metric_tracer_handle);
      lzt::deactivate_metric_groups(device);
      lzt::destroy_event(notification_event);
      lzt::destroy_event_pool(notification_event_pool);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

LZT_TEST_F(zetMetricTracerTest,
           GivenDecoderIsCreatedWhenTracerIsDestroyedThenDecoderRemainsActive) {
  ze_result_t result;

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);
    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());
    zet_metric_tracer_exp_handle_t metric_tracer_handle = nullptr;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    uint32_t decodable_metric_count =
        lzt::metric_decoder_get_decodable_metrics_count(metric_decoder_handle);
    std::vector<zet_metric_handle_t> decodable_metric_handles(
        decodable_metric_count);
    lzt::metric_decoder_get_decodable_metrics(metric_decoder_handle,
                                              &decodable_metric_handles);
    decodable_metric_count = to_u32(decodable_metric_handles.size());
    for (uint32_t i = 0; i < decodable_metric_count; i++) {
      bool valid_type;
      zet_metric_properties_t decodable_metric_properties;
      lzt::get_metric_properties(decodable_metric_handles[i],
                                 &decodable_metric_properties);
      size_t metric_name_string_length;
      metric_name_string_length =
          strnlen(decodable_metric_properties.name, ZET_MAX_METRIC_NAME);
      EXPECT_GT(metric_name_string_length, 0)
          << "the name string for this metric is of zero length";
      EXPECT_LT(metric_name_string_length, ZET_MAX_METRIC_NAME)
          << "the name string for this metric is not null terminated";
      valid_type =
          lzt::verify_value_type(decodable_metric_properties.resultType);
      EXPECT_TRUE(valid_type)
          << "metric properties for metric " << decodable_metric_properties.name
          << " has an incorrect resultType field "
          << decodable_metric_properties.resultType;
      valid_type =
          lzt::verify_metric_type(decodable_metric_properties.metricType);
      EXPECT_TRUE(valid_type)
          << "metric properties for metric " << decodable_metric_properties.name
          << " has an invalid metricType field "
          << decodable_metric_properties.metricType;
    }
    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::deactivate_metric_groups(device);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenDecoderIsCreatedWhenRequestingMoreDecodableMetricsThanAvailableThenReturnOnlyWhatIsAvailable) {
  /* When allocating a buffer for more decodable metrics than available,
   * ensure that writes happen only for the actual number of available decodable
   * metrics */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {

    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    /* read data */
    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, raw_data.data()));
    ASSERT_NE(raw_data_size, 0)
        << "zetMetricTracerReadDataExp returned no data";
    raw_data.resize(raw_data_size);
    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);

    uint32_t decodable_metric_count{};
    EXPECT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
        metric_decoder_handle, &decodable_metric_count, nullptr));
    EXPECT_NE(0u, decodable_metric_count)
        << "zetMetricDecoderGetDecodableMetricsExp reports that there are no "
           "decodable metrics";

    if (decodable_metric_count != 0) {
      LOG_DEBUG << "found " << decodable_metric_count << " decodable metrics";
      const uint32_t additional_count = 8;
      const zet_metric_handle_t decodable_metric_init = nullptr;
      decodable_metric_count += additional_count;

      std::vector<zet_metric_handle_t> decodable_metric_handles(
          decodable_metric_count, decodable_metric_init);
      EXPECT_ZE_RESULT_SUCCESS(zetMetricDecoderGetDecodableMetricsExp(
          metric_decoder_handle, &decodable_metric_count,
          decodable_metric_handles.data()));

      for (auto itr = decodable_metric_handles.size() - additional_count;
           itr < decodable_metric_handles.size(); ++itr) {
        EXPECT_EQ(nullptr, decodable_metric_handles[itr])
            << "zetMetricDecoderGetDecodableMetricsExp should not return more "
               "decodable metrics than are available";
      }

      /* decode data */
      uint32_t set_count{};
      uint32_t metric_entry_count{};
      lzt::metric_tracer_decode_get_various_counts(
          metric_decoder_handle, &raw_data_size, &raw_data,
          decodable_metric_count, &decodable_metric_handles, &set_count,
          &metric_entry_count);
      EXPECT_NE(0u, metric_entry_count)
          << "zetMetricTracerDecodeExp reports that there are no metric "
             "entries to be decoded";

      if (metric_entry_count != 0) {
        std::vector<uint32_t> metric_entries_per_set_count(set_count);
        std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count);
        lzt::metric_tracer_decode(metric_decoder_handle, &raw_data_size,
                                  &raw_data, decodable_metric_count,
                                  &decodable_metric_handles, &set_count,
                                  &metric_entries_per_set_count,
                                  &metric_entry_count, &metric_entries);

        uint32_t set_entry_start = 0;
        for (uint32_t set_index = 0; set_index < set_count; set_index++) {
          LOG_DEBUG << "Set number: " << set_index << " Entries in set: "
                    << metric_entries_per_set_count[set_index] << "\n";

          for (uint32_t index = set_entry_start;
               index <
               set_entry_start + metric_entries_per_set_count[set_index];
               index++) {
            auto &metric_entry = metric_entries[index];
            EXPECT_LT(metric_entry.metricIndex, decodable_metric_count);
            if (metric_entry.onSubdevice) {
              EXPECT_EQ(metric_entry.subdeviceId, set_index);
            }
          }
          set_entry_start += metric_entries_per_set_count[set_index];
        }
      }
    }
    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::reset_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenDecoderIsCreatedWhenRequestingMoreMetricEntriesThanAvailableThenReturnOnlyWhatIsAvailable) {
  /* When allocating a buffer for more metric entries than available, ensure
   * that writes happen only for the actual number of available metric entries
   */
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {

    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(command_list, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(command_list);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    /* read data */
    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, raw_data.data()));
    ASSERT_NE(raw_data_size, 0)
        << "zetMetricTracerReadDataExp returned no data";
    raw_data.resize(raw_data_size);
    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;

    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);
    uint32_t decodable_metric_count =
        lzt::metric_decoder_get_decodable_metrics_count(metric_decoder_handle);
    std::vector<zet_metric_handle_t> decodable_metric_handles(
        decodable_metric_count);
    lzt::metric_decoder_get_decodable_metrics(metric_decoder_handle,
                                              &decodable_metric_handles);
    decodable_metric_count = to_u32(decodable_metric_handles.size());

    /* decode data */
    uint32_t set_count{};
    uint32_t metric_entry_count{};
    EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerDecodeExp(
        metric_decoder_handle, &raw_data_size, raw_data.data(),
        decodable_metric_count, decodable_metric_handles.data(), &set_count,
        nullptr, &metric_entry_count, nullptr));
    LOG_DEBUG << "decodable metric entry count: " << metric_entry_count;
    LOG_DEBUG << "set count: " << set_count;
    EXPECT_NE(0u, metric_entry_count)
        << "zetMetricTracerDecodeExp reports that there are no metric entries "
           "to be decoded";

    if (metric_entry_count != 0) {
      const uint32_t additional_entries = 8;
      zet_metric_entry_exp_t metric_entries_init{};
      metric_entries_init.metricIndex = 0xBEEF;
      metric_entry_count += additional_entries;

      std::vector<uint32_t> metric_entries_per_set_count(set_count, 0u);
      std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count,
                                                         metric_entries_init);
      EXPECT_ZE_RESULT_SUCCESS(zetMetricTracerDecodeExp(
          metric_decoder_handle, &raw_data_size, raw_data.data(),
          decodable_metric_count, decodable_metric_handles.data(), &set_count,
          metric_entries_per_set_count.data(), &metric_entry_count,
          metric_entries.data()));

      for (auto itr = metric_entries.size() - additional_entries;
           itr < metric_entries.size(); ++itr) {
        EXPECT_EQ(metric_entries_init.metricIndex,
                  metric_entries[itr].metricIndex)
            << "zetMetricTracerDecodeExp should not return more metric entries "
               "than are available";
      }
    }
    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::reset_command_list(command_list);
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
  }
}

LZT_TEST_F(zetMetricTracerTest,
           GivenDecoderIsCreatedThenEnsureAllMetricEntriesTimeStampsAreValid) {

  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);
    void *a_buffer, *b_buffer, *c_buffer;
    ze_group_count_t tg;
    ze_kernel_handle_t function = get_matrix_multiplication_kernel(
        device, &tg, &a_buffer, &b_buffer, &c_buffer, 128);
    lzt::append_launch_function(commandList, function, &tg, nullptr, 0,
                                nullptr);
    lzt::close_command_list(commandList);

    ze_event_pool_handle_t notification_event_pool;
    notification_event_pool = lzt::create_event_pool(
        lzt::get_default_context(), 1, ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
    ze_event_handle_t notification_event;
    ze_event_desc_t notification_event_descriptor = {
        ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST,
        ZE_EVENT_SCOPE_FLAG_DEVICE};
    notification_event = lzt::create_event(notification_event_pool,
                                           notification_event_descriptor);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_GT(num_grp_handles, 0u);
    lzt::activate_metric_groups(device, num_grp_handles, grp_handles.data());
    uint64_t host_timestamp_start, device_timestamp_start;
    std::tie(host_timestamp_start, device_timestamp_start) =
        lzt::get_global_timestamps(device);
    LOG_DEBUG << "host_timestamp_start = " << host_timestamp_start
              << " device_timestamp_start = " << device_timestamp_start;

    zet_metric_tracer_exp_handle_t metric_tracer_handle;
    lzt::metric_tracer_create(
        lzt::get_default_context(), device, num_grp_handles, grp_handles.data(),
        &tracer_descriptor, notification_event, &metric_tracer_handle);
    lzt::metric_tracer_enable(metric_tracer_handle, true);
    LOG_DEBUG << "execute workload";
    lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);
    LOG_DEBUG << "synchronize with completion of workload";
    lzt::synchronize(commandQueue, std::numeric_limits<uint64_t>::max());
    LOG_DEBUG << "synchronize on the tracer notification event";
    lzt::event_host_synchronize(notification_event,
                                std::numeric_limits<uint64_t>::max());
    lzt::metric_tracer_disable(metric_tracer_handle, true);
    /* read data */
    size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
    std::vector<uint8_t> raw_data(raw_data_size, 0);
    ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
        metric_tracer_handle, &raw_data_size, raw_data.data()));
    ASSERT_NE(raw_data_size, 0)
        << "zetMetricTracerReadDataExp returned no data";
    raw_data.resize(raw_data_size);
    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;

    lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);
    uint32_t decodable_metric_count =
        lzt::metric_decoder_get_decodable_metrics_count(metric_decoder_handle);
    std::vector<zet_metric_handle_t> decodable_metric_handles(
        decodable_metric_count);
    lzt::metric_decoder_get_decodable_metrics(metric_decoder_handle,
                                              &decodable_metric_handles);
    decodable_metric_count = to_u32(decodable_metric_handles.size());
    /* decode data */
    uint32_t metric_entry_count = 0;
    uint32_t set_count = 0;
    lzt::metric_tracer_decode_get_various_counts(
        metric_decoder_handle, &raw_data_size, &raw_data,
        decodable_metric_count, &decodable_metric_handles, &set_count,
        &metric_entry_count);
    std::vector<uint32_t> metric_entries_per_set_count(set_count);
    std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count);
    lzt::metric_tracer_decode(metric_decoder_handle, &raw_data_size, &raw_data,
                              decodable_metric_count, &decodable_metric_handles,
                              &set_count, &metric_entries_per_set_count,
                              &metric_entry_count, &metric_entries);
    EXPECT_NE(0u, metric_entry_count)
        << "zetMetricTracerDecodeExp reports that there are no metric entries "
           "to be decoded";

    if (metric_entry_count != 0) {
      uint64_t metric_entry_timestamp_start = 0;
      uint64_t metric_entry_timestamp_end = 0;
      metric_entry_timestamp_start = metric_entries[0].timeStamp;
      LOG_DEBUG << "metric entry timestamp start "
                << metric_entry_timestamp_start;
      metric_entry_timestamp_end =
          metric_entries[metric_entry_count - 1].timeStamp;
      LOG_DEBUG << "metric entry timestamp end " << metric_entry_timestamp_end;

      uint64_t host_timestamp_end, device_timestamp_end;
      std::tie(host_timestamp_end, device_timestamp_end) =
          lzt::get_global_timestamps(device);
      LOG_DEBUG << "host_timestamp_end = " << host_timestamp_end
                << " device_timestamp_end = " << device_timestamp_end;
      EXPECT_GT(metric_entry_timestamp_start, device_timestamp_start)
          << "first metric entry timestamp should be greater than first "
             "device timestamp";
      EXPECT_LT(metric_entry_timestamp_end, device_timestamp_end)
          << "last metric entry timestamp should be lesser than last device "
             "timestamp";
    }
    lzt::metric_decoder_destroy(metric_decoder_handle);
    lzt::metric_tracer_destroy(metric_tracer_handle);
    lzt::deactivate_metric_groups(device);
    lzt::free_memory(a_buffer);
    lzt::free_memory(b_buffer);
    lzt::free_memory(c_buffer);
    lzt::reset_command_list(commandList);
    lzt::destroy_event(notification_event);
    lzt::destroy_event_pool(notification_event_pool);
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

LZT_TEST_F(
    zetMetricTracerTest,
    GivenMetricGroupSupportsTracerAndDmaBufAccessWhenWritingToDmaBufFromKernelThenEventIsGenerated) {
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    lzt::display_device_properties(device);

    auto &grp_handles =
        device_with_metric_group_handles.activatable_metric_group_handle_list;
    uint32_t num_grp_handles = to_u32(grp_handles.size());
    ASSERT_NE(0u, num_grp_handles);

    std::vector<zet_metric_group_handle_t> dma_buf_metric_group_handles;
    lzt::get_metric_groups_supporting_dma_buf(
        grp_handles, ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED,
        dma_buf_metric_group_handles);
    uint32_t num_buf_handles = to_u32(dma_buf_metric_group_handles.size());
    ASSERT_NE(0u, num_buf_handles);

    uint32_t dma_buf_validation_count{};
    ze_command_queue_handle_t command_queue = lzt::create_command_queue(device);
    zet_command_list_handle_t command_list = lzt::create_command_list(device);
    auto module = lzt::create_module(device, "copy_module.spv");
    auto kernel = lzt::create_function(module, "copy_data");
    uint8_t *src_buf, *dst_buf;
    const uint8_t src_buf_data = 0xab;

    for (uint32_t i = 0; i < num_buf_handles; ++i) {
      int fd;
      size_t size;
      lzt::metric_get_dma_buf_fd_and_size(dma_buf_metric_group_handles[i], fd,
                                          size);
      size_t offset = 0;
      lzt::activate_metric_groups(device, num_buf_handles,
                                  dma_buf_metric_group_handles.data());

      src_buf =
          static_cast<uint8_t *>(lzt::allocate_shared_memory(size, device));
      std::fill(src_buf, src_buf + (size / sizeof(src_buf_data)), src_buf_data);
      dst_buf = static_cast<uint8_t *>(lzt::metric_map_dma_buf_fd_to_memory(
          device, lzt::get_default_context(), fd, size, offset));
      lzt::set_argument_value(kernel, 0, sizeof(src_buf), &src_buf);
      lzt::set_argument_value(kernel, 1, sizeof(dst_buf), &dst_buf);
      lzt::set_argument_value(kernel, 2, sizeof(int), &offset);
      lzt::set_argument_value(kernel, 3, sizeof(int), &size);
      lzt::set_group_size(kernel, 1, 1, 1);
      ze_group_count_t group_count = {1, 1, 1};
      lzt::append_launch_function(command_list, kernel, &group_count, nullptr,
                                  0, nullptr);
      lzt::close_command_list(command_list);

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      lzt::metric_tracer_create(
          lzt::get_default_context(), device, num_buf_handles,
          dma_buf_metric_group_handles.data(), &tracer_descriptor, nullptr,
          &metric_tracer_handle);
      lzt::metric_tracer_enable(metric_tracer_handle, true);
      LOG_DEBUG << "execute workload";
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      LOG_DEBUG << "synchronize with completion of workload";
      lzt::synchronize(command_queue, std::numeric_limits<uint64_t>::max());
      lzt::metric_tracer_disable(metric_tracer_handle, true);
      /* read data */
      size_t raw_data_size = 1024 * 1024; /* 1MB buffer */
      std::vector<uint8_t> raw_data(raw_data_size, 0);
      ASSERT_ZE_RESULT_SUCCESS(zetMetricTracerReadDataExp(
          metric_tracer_handle, &raw_data_size, raw_data.data()));
      ASSERT_NE(raw_data_size, 0)
          << "zetMetricTracerReadDataExp returned no data";
      raw_data.resize(raw_data_size);
      zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;

      lzt::metric_decoder_create(metric_tracer_handle, &metric_decoder_handle);
      uint32_t decodable_metric_count =
          lzt::metric_decoder_get_decodable_metrics_count(
              metric_decoder_handle);
      std::vector<zet_metric_handle_t> decodable_metric_handles(
          decodable_metric_count);
      lzt::metric_decoder_get_decodable_metrics(metric_decoder_handle,
                                                &decodable_metric_handles);
      decodable_metric_count = to_u32(decodable_metric_handles.size());
      /* decode data */
      uint32_t metric_entry_count = 0;
      uint32_t set_count = 0;
      lzt::metric_tracer_decode_get_various_counts(
          metric_decoder_handle, &raw_data_size, &raw_data,
          decodable_metric_count, &decodable_metric_handles, &set_count,
          &metric_entry_count);
      std::vector<uint32_t> metric_entries_per_set_count(set_count);
      std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count);
      lzt::metric_tracer_decode(
          metric_decoder_handle, &raw_data_size, &raw_data,
          decodable_metric_count, &decodable_metric_handles, &set_count,
          &metric_entries_per_set_count, &metric_entry_count, &metric_entries);
      EXPECT_NE(0u, metric_entry_count)
          << "zetMetricTracerDecodeExp reports that there are no metric "
             "entries to be decoded";

      if (metric_entry_count != 0) {
        dma_buf_validation_count++;
        uint8_t exported_memory_data = to_u8(metric_entries[0].value.ui32);
        EXPECT_EQ(src_buf_data, exported_memory_data);
      }
      lzt::metric_decoder_destroy(metric_decoder_handle);
      lzt::metric_tracer_destroy(metric_tracer_handle);
      lzt::deactivate_metric_groups(device);
      lzt::reset_command_list(command_list);
    }
    lzt::destroy_command_queue(command_queue);
    lzt::destroy_command_list(command_list);
    EXPECT_NE(0, dma_buf_validation_count);
  }
}

} // namespace
