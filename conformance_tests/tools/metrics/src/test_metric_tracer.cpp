/*
 *
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>

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

TEST_F(
    zetMetricTracerTest,
    GivenActivatedMetricGroupsWhenTracerIsCreatedWithOneOrMultipleMetricsGroupsThenTracerCreateSucceeds) {

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    for (int32_t i = 0; i < device_with_metric_group_handles
                                .activatable_metric_group_handle_list.size();
         i++) {

      lzt::activate_metric_groups(
          device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricTracerCreateExp failed";

      zet_metric_tracer_exp_handle_t metric_tracer_handle1;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle1);
      EXPECT_NE(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp of a second tracer handle failed "
             "with "
             "error code "
          << result;

      result = zetMetricTracerDestroyExp(metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp failed";
      lzt::deactivate_metric_groups(device);
    }
  }
}

TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWithOneOrMoreActivatedMetricGroupWhenTracerIsEnabledSynchronouslyThenExpectTracerEnableToSucceed) {

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    for (int32_t i = 0; i < device_with_metric_group_handles
                                .activatable_metric_group_handle_list.size();
         i++) {

      lzt::activate_metric_groups(
          device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle0;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle0);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp failed creating the first tracer "
             "for this "
             "device";

      result = zetMetricTracerEnableExp(metric_tracer_handle0, true);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerEnableExp synchronously failed for first "
             "tracer on "
             "this device";

      result = zetMetricTracerDestroyExp(metric_tracer_handle0);
      ASSERT_NE(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp succeeded on a tracer that is "
             "still enabled";

      result = zetMetricTracerDisableExp(metric_tracer_handle0, true);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDisableExp synchronously failed for the "
             "first tracer "
             "handle for this device";

      result = zetMetricTracerDestroyExp(metric_tracer_handle0);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp failed the first tracer "
             "handle for this device";

      lzt::deactivate_metric_groups(device);
    }
  }
}

TEST_F(
    zetMetricTracerTest,
    GivenTracerIsCreatedWithOneOrMoreActivatedMetricGroupWhenTracerIsEnabledAndDisabledAsynchronouslyThenExpectTracerEnableAndDisableToSucceed) {

  constexpr int32_t number_of_retries = 5;
  constexpr int32_t retry_wait_milliseconds = 5;
  ze_result_t result;

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);

    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    for (int32_t i = 0; i < device_with_metric_group_handles
                                .activatable_metric_group_handle_list.size();
         i++) {

      lzt::activate_metric_groups(
          device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp first tracer create failed";

      result = zetMetricTracerEnableExp(metric_tracer_handle, false);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerEnableExp on the first tracer handle "
             "asynchronously "
             "failed";

      int32_t j = 0;
      do {
        size_t raw_data_size = 0;
        result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                            &raw_data_size, nullptr);
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

      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerReadDataExp has failed with error code " << result;

      result = zetMetricTracerDisableExp(metric_tracer_handle, false);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDisableExp failed on first tracer handle";

      int32_t k = 0;
      std::vector<uint8_t> raw_data_buffer;
      do {
        size_t raw_data_size = 0;
        result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                            &raw_data_size, nullptr);
        if (result == ZE_RESULT_SUCCESS) {
          if (raw_data_size != 0) {
            size_t new_raw_data_size = raw_data_size;
            raw_data_buffer.resize(raw_data_size);
            result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                                &new_raw_data_size,
                                                raw_data_buffer.data());
            ASSERT_EQ(result, ZE_RESULT_SUCCESS)
                << "zetMetricTracerReadDataExp() with rawDataSize value "
                << raw_data_size << " and data buffer has failed";
            ASSERT_EQ(raw_data_size, new_raw_data_size)
                << "zetMetricTracerReadDataExp called with a non-zero "
                   "rawDataSize "
                << raw_data_size << " modified the rawDataSize parameter "
                << new_raw_data_size;
          }
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

      result = zetMetricTracerDestroyExp(metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp failed the first tracer handle "
             "for "
             "this device";

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

class zetMetricTracerParameterizedReadTestFixture
    : public zetMetricTracerTest,
      public ::testing::WithParamInterface<bool> {};

TEST_P(
    zetMetricTracerParameterizedReadTestFixture,
    GivenAsynchronouslyOrSynchronouslyEnabledAndDisabledTracerWithOneOrMoreMetricsGroupsAndWorkloadExecutionThenExpectTracerReadsToSucceed) {

  constexpr int32_t number_of_retries = 5;
  constexpr int32_t retry_wait_milliseconds = 5;
  bool test_is_synchronous = GetParam();
  ze_result_t result;

  std::string test_mode;
  if (test_is_synchronous) {
    test_mode = "Synchronous";
  } else {
    test_mode = "Asynchronous";
  }

  LOG_INFO << "testing zetMetricTracerReadDataExp with " << test_mode
           << " tracer Enable and Disable";

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);

    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    for (int32_t i = 0; i < device_with_metric_group_handles
                                .activatable_metric_group_handle_list.size();
         i++) {

      lzt::activate_metric_groups(
          device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data());

      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, nullptr, &metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp failed creating the first tracer "
             "for this "
             "device";

      result =
          zetMetricTracerEnableExp(metric_tracer_handle, test_is_synchronous);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerEnableExp in " << test_mode
          << " mode failed for first tracer on this device";

      if (!test_is_synchronous) {
        int32_t j = 0;
        size_t raw_data_size = 0;
        do {
          result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                              &raw_data_size, nullptr);
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

        ASSERT_EQ(result, ZE_RESULT_SUCCESS)
            << "zetMetricTracerReadDataExp has failed with an unexpetced "
               "error";

        if (raw_data_size != 0) {
          size_t new_raw_data_size;
          new_raw_data_size = raw_data_size;
          std::vector<uint8_t> raw_data_buffer(raw_data_size);
          result = zetMetricTracerReadDataExp(
              metric_tracer_handle, &new_raw_data_size, raw_data_buffer.data());
          ASSERT_EQ(result, ZE_RESULT_SUCCESS)
              << "zetMetricTracerReadDataExp called with non-zero "
                 "rawDataSize "
                 "and a properly sized data buffer failed with error code "
              << result;
          ASSERT_EQ(raw_data_size, new_raw_data_size)
              << "zetMetricTracerReadDataExp called with non-zero "
                 "rawDataSize "
                 "value "
              << raw_data_size << "returned a different data size "
              << new_raw_data_size;
        }
      }

      executeMatrixMultiplyWorkload(device, commandQueue, commandList);

      size_t raw_data_size = 0;
      result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                          nullptr);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerReadDataExp with null data buffer pointer on "
             "an "
             "enabled "
             "tracer has failed";
      ASSERT_NE(raw_data_size, 0) << "After executing a workload, "
                                     "zetMetricTracerReadDataExp with an "
                                     "enabled tracer and null data "
                                     "pointer returned 0 raw data size";

      size_t enabled_read_data_size = raw_data_size / 2;
      std::vector<uint8_t> enabled_raw_data(enabled_read_data_size);

      result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                          &enabled_read_data_size,
                                          enabled_raw_data.data());
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerReadDataExp with non-null data buffer on an "
             "enabled "
             "tracer has failed";
      ASSERT_NE(enabled_read_data_size, 0)
          << "zetMetricTracerReadDataExp on an enabled "
             "tracer returned zero data size";

      result =
          zetMetricTracerDisableExp(metric_tracer_handle, test_is_synchronous);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDisableExp synchronously failed for the "
             "first tracer "
             "handle for this device";

      size_t disabled_read_data_size = raw_data_size - enabled_read_data_size;
      std::vector<uint8_t> disabled_raw_data(disabled_read_data_size);

      result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                          &disabled_read_data_size,
                                          disabled_raw_data.data());
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerReadDataExp with non-null data buffer and "
             "disabled "
             "tracer has failed";
      ASSERT_NE(disabled_read_data_size, 0)
          << "zetMetricTracerReadDataExp with "
             "non-null data buffer and disabled "
             "tracer has returned no data";
      if (!test_is_synchronous) {
        int32_t k = 0;
        do {
          size_t raw_data_size = 0;
          result = zetMetricTracerReadDataExp(metric_tracer_handle,
                                              &raw_data_size, nullptr);
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

      result = zetMetricTracerDestroyExp(metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp failed the first tracer handle "
             "for "
             "this device";

      lzt::deactivate_metric_groups(device);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

INSTANTIATE_TEST_SUITE_P(ReadTestWithAsynchronousOrSynchronousEnableAndDisable,
                         zetMetricTracerParameterizedReadTestFixture,
                         ::testing::Values(true));

TEST_F(zetMetricTracerTest,
       GivenTracerIsCreatedThenDecoderCreateAndDeleteSucceed) {

  ze_result_t result;

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);
    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    lzt::activate_metric_groups(
        device,
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .size(),
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle = nullptr;
    result = zetMetricTracerCreateExp(
        lzt::get_default_context(), device,
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .size(),
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricTracerCreateExp failed";
    ASSERT_NE(metric_tracer_handle, nullptr);

    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    result =
        zetMetricDecoderCreateExp(metric_tracer_handle, &metric_decoder_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricDecoderCreateExp failed";
    ASSERT_NE(metric_decoder_handle, nullptr);

    result = zetMetricDecoderDestroyExp(metric_decoder_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricDecoderDestroyExp failed";

    result = zetMetricTracerDestroyExp(metric_tracer_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricTracerDestroyExp failed";
    lzt::deactivate_metric_groups(device);
  }
}

TEST_F(zetMetricTracerTest,
       GivenDecoderIsCreatedThenGetDecodableMetricsSucceeds) {

  ze_result_t result;

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;

    lzt::display_device_properties(device);
    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    lzt::activate_metric_groups(
        device,
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .size(),
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .data());

    zet_metric_tracer_exp_handle_t metric_tracer_handle = nullptr;
    result = zetMetricTracerCreateExp(
        lzt::get_default_context(), device,
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .size(),
        device_with_metric_group_handles.activatable_metric_group_handle_list
            .data(),
        &tracer_descriptor, nullptr, &metric_tracer_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricTracerCreateExp failed";
    ASSERT_NE(metric_tracer_handle, nullptr);

    zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
    result =
        zetMetricDecoderCreateExp(metric_tracer_handle, &metric_decoder_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricDecoderCreateExp failed";
    ASSERT_NE(metric_decoder_handle, nullptr);

    uint32_t num_decodable_metrics = 0;
    result = zetMetricDecoderGetDecodableMetricsExp(
        metric_decoder_handle, &num_decodable_metrics, nullptr);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS)
        << "zetMetricDecoderGetDecodableMetricsExp call failed when "
           "querying for "
           "the number of decodable metrics";
    ASSERT_GT(num_decodable_metrics, 0u)
        << "zetMetricDecoderGetDecodableMetricsExp reports that there are no "
           "decodable metrics";

    uint32_t new_num_decodable_metrics = num_decodable_metrics + 1;
    std::vector<zet_metric_handle_t> metric_handles(new_num_decodable_metrics);
    result = zetMetricDecoderGetDecodableMetricsExp(metric_decoder_handle,
                                                    &new_num_decodable_metrics,
                                                    metric_handles.data());
    ASSERT_EQ(result, ZE_RESULT_SUCCESS)
        << "zetMetricDecoderGetDecodableMetricsExp failed retrieving the "
           "list of "
           "handles for decodable metrics";
    ASSERT_EQ(new_num_decodable_metrics, num_decodable_metrics)
        << "zetMetricDecoderGetDecodableMetricsExp reported an unexpected "
           "value "
           "for the count of decodable metrics";

    ASSERT_GT(num_decodable_metrics, 0u);
    for (int i = 0; i < num_decodable_metrics; i++) {
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

    result = zetMetricDecoderDestroyExp(metric_decoder_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricDecoderDestroyExp failed";

    result = zetMetricTracerDestroyExp(metric_tracer_handle);
    ASSERT_EQ(result, ZE_RESULT_SUCCESS) << "zetMetricTracerDestroyExp failed";
    lzt::deactivate_metric_groups(device);
  }
}

TEST_F(
    zetMetricTracerTest,
    GivenDecoderIsCreatedAndWorkloadExecutedAndTracerIsEnabledThenDecoderSucceedsDecodingMetrics) {

  if (tracer_supporting_devices_list.size() == 0) {
    GTEST_SKIP()
        << "No devices that have metric groups of type Tracer were found";
  }
  for (auto &device_with_metric_group_handles :
       tracer_supporting_devices_list) {
    device = device_with_metric_group_handles.device;
    ze_result_t result;

    lzt::display_device_properties(device);

    ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
    zet_command_list_handle_t commandList = lzt::create_command_list(device);

    ASSERT_GT(device_with_metric_group_handles
                  .activatable_metric_group_handle_list.size(),
              0u);
    for (int32_t i = 0; i < device_with_metric_group_handles
                                .activatable_metric_group_handle_list.size();
         i++) {

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

      lzt::activate_metric_groups(
          device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data());

      LOG_DEBUG << "create tracer";
      zet_metric_tracer_exp_handle_t metric_tracer_handle;
      result = zetMetricTracerCreateExp(
          lzt::get_default_context(), device, i + 1,
          device_with_metric_group_handles.activatable_metric_group_handle_list
              .data(),
          &tracer_descriptor, notification_event, &metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerCreateExp failed creating the tracer for "
             "this "
             "device";

      LOG_DEBUG << "enable tracer synchronously";
      result = zetMetricTracerEnableExp(metric_tracer_handle, true);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerEnableExp in synchronous "
          << " mode failed for the tracer on this device";

      LOG_DEBUG << "create tracer decoder";
      zet_metric_decoder_exp_handle_t metric_decoder_handle = nullptr;
      result = zetMetricDecoderCreateExp(metric_tracer_handle,
                                         &metric_decoder_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricDecoderCreateExp failed for this device";
      ASSERT_NE(metric_decoder_handle, nullptr)
          << "zetMetricDecoderCreateExp returned a NULL handle";

      uint32_t decodable_metric_count = 0;
      result = zetMetricDecoderGetDecodableMetricsExp(
          metric_decoder_handle, &decodable_metric_count, nullptr);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricDecoderGetDecodableMetricsExp call failed when "
             "querying "
             "for "
             "the number of decodable metrics";
      ASSERT_NE(decodable_metric_count, 0)
          << "zetMetricDecoderGetDecodableMetricsExp reports there a NO "
             "decodable metrics";

      std::vector<zet_metric_handle_t> decodable_metric_handles(
          decodable_metric_count);
      result = zetMetricDecoderGetDecodableMetricsExp(
          metric_decoder_handle, &decodable_metric_count,
          decodable_metric_handles.data());
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricDecoderGetDecodableMetricsExp failed retrieving "
             "the list "
             "of "
             "handles for decodable metrics";
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

      uint64_t elapsedTime =
          std::chrono::duration_cast<std::chrono::nanoseconds>(endTime -
                                                               startTime)
              .count();
      LOG_DEBUG << "resumed after synchronizing on the tracer notification "
                   "event, blocked for "
                << elapsedTime << " nanoseconds";

      if (result != ZE_RESULT_SUCCESS) {
        LOG_WARNING << "zeEventHostSynchronize on notification event waiting "
                       "for trace data failed with error code "
                    << result;
      }

      /* read data */
      size_t raw_data_size = 0;
      result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                          nullptr);
      LOG_DEBUG << "tracer read for  Request Size: " << raw_data_size;
      ASSERT_EQ(result, ZE_RESULT_SUCCESS);
      ASSERT_GT(raw_data_size, 0);
      std::vector<uint8_t> raw_data(raw_data_size, 0);

      result = zetMetricTracerReadDataExp(metric_tracer_handle, &raw_data_size,
                                          raw_data.data());
      LOG_DEBUG << "tracer read for data: result " << result;
      ASSERT_EQ(result, ZE_RESULT_SUCCESS);

      /* Decode Data */
      uint32_t metric_entry_count = 0;
      uint32_t set_count = 0;

      result = zetMetricTracerDecodeExp(
          metric_decoder_handle, &raw_data_size, raw_data.data(),
          decodable_metric_count, decodable_metric_handles.data(), &set_count,
          nullptr, &metric_entry_count, nullptr);

      ASSERT_EQ(result, ZE_RESULT_SUCCESS);
      LOG_DEBUG << "Decoded Metric Entry Count: " << metric_entry_count;
      LOG_DEBUG << "Set Count: " << set_count;

      std::vector<uint32_t> metric_entries_per_set_count(set_count);
      std::vector<zet_metric_entry_exp_t> metric_entries(metric_entry_count);

      result = zetMetricTracerDecodeExp(
          metric_decoder_handle, &raw_data_size, raw_data.data(),
          decodable_metric_count, decodable_metric_handles.data(), &set_count,
          metric_entries_per_set_count.data(), &metric_entry_count,
          metric_entries.data());

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

      result = zetMetricDecoderDestroyExp(metric_decoder_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricDecoderDestroyExp failed the first tracer "
             "handle for this device";

      result = zetMetricTracerDisableExp(metric_tracer_handle, true);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDisableExp synchronously failed for the "
             "first tracer "
             "handle for this device";

      result = zetMetricTracerDestroyExp(metric_tracer_handle);
      ASSERT_EQ(result, ZE_RESULT_SUCCESS)
          << "zetMetricTracerDestroyExp failed the first tracer "
             "handle for this device";

      lzt::deactivate_metric_groups(device);
      lzt::destroy_event(notification_event);
      lzt::destroy_event_pool(notification_event_pool);
    }
    lzt::destroy_command_queue(commandQueue);
    lzt::destroy_command_list(commandList);
  }
}

} // namespace
