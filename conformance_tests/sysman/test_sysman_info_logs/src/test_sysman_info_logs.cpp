/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

constexpr uint64_t read_timeout_ms = 1000;
constexpr uint32_t min_read_size = 64 * 1024;
constexpr uint32_t min_read_record_count = 64;
constexpr uint32_t requested_buffer_size_in_kb = 1024;
constexpr uint8_t sentinel_byte = 0xA5;
constexpr const char *named_instance_name = "lzt_info_log_cts_instance";

#ifdef USE_ZESINIT
#define INFO_LOG_TEST InfoLogZesTest
using SysmanBaseTest = lzt::ZesSysmanCtsClass;
#else // USE_ZESINIT
#define INFO_LOG_TEST InfoLogTest
using SysmanBaseTest = lzt::SysmanCtsClass;
#endif // USE_ZESINIT

class INFO_LOG_TEST : public SysmanBaseTest {
protected:
  zes_driver_handle_t driver = nullptr;
  std::vector<zes_info_log_handle_t> info_log_handles;

  void SetUp() override {
#ifdef USE_ZESINIT
    driver = lzt::get_default_zes_driver();
#else  // USE_ZESINIT
    driver = lzt::get_default_driver();
#endif // USE_ZESINIT
    uint32_t count = 0;
    ze_result_t result = zesDriverEnumInfoLogsExt(driver, &count, nullptr);
    if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
      GTEST_SKIP() << "Info logs extension is not supported";
    }
    ASSERT_ZE_RESULT_SUCCESS(result);
    if (count == 0) {
      GTEST_SKIP() << "No info log handles found";
    }
    info_log_handles = lzt::get_info_log_handles(driver, count);
    ASSERT_EQ(info_log_handles.size(), count);
  }
};

struct InfoLogRecords {
  ze_result_t result = ZE_RESULT_SUCCESS;
  uint32_t size = 0;
  uint32_t record_count = 0;
  std::vector<uint8_t> data;
  std::vector<zes_info_log_metadata_ext_t> descriptors;
  zes_info_log_read_status_ext_t read_status = {
      ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT, nullptr};
};

zes_info_log_metadata_ext_t create_metadata() {
  zes_info_log_metadata_ext_t metadata = {};
  metadata.stype = ZES_STRUCTURE_TYPE_INFO_LOG_METADATA_EXT;
  metadata.pNext = nullptr;
  return metadata;
}

// Makes a query call to size the buffers, then reads (or peeks) the records.
InfoLogRecords collect_info_log_records(zes_info_log_instance_handle_t instance,
                                        bool peek, uint64_t timeout) {
  InfoLogRecords records;
  uint32_t query_size = 0;
  uint32_t query_count = 0;
  zes_info_log_read_status_ext_t query_status = {
      ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT, nullptr};
  records.result =
      lzt::read_info_log_records(instance, peek, timeout, query_size, nullptr,
                                 query_count, nullptr, &query_status);
  EXPECT_TRUE(lzt::is_info_log_read_successful(records.result))
      << "Query call returned " << lzt::to_string(records.result);

  records.size = std::max(query_size, min_read_size);
  records.record_count = std::max(query_count, min_read_record_count);
  records.data.assign(records.size, 0);
  records.descriptors.assign(records.record_count, create_metadata());
  const uint32_t size_capacity = records.size;
  const uint32_t record_capacity = records.record_count;
  records.result = lzt::read_info_log_records(
      instance, peek, timeout, records.size, records.data.data(),
      records.record_count, records.descriptors.data(), &records.read_status);
  EXPECT_TRUE(lzt::is_info_log_read_successful(records.result))
      << "Read call returned " << lzt::to_string(records.result);
  EXPECT_LE(records.size, size_capacity);
  EXPECT_LE(records.record_count, record_capacity);
  return records;
}

void validate_info_log_records(const InfoLogRecords &records) {
  if (records.record_count == 0) {
    EXPECT_EQ(records.size, 0u);
    return;
  }
  ASSERT_LE(records.record_count, records.descriptors.size());
  std::vector<zes_info_log_metadata_ext_t> sorted(records.descriptors.begin(),
                                                  records.descriptors.begin() +
                                                      records.record_count);
  for (const auto &metadata : sorted) {
    EXPECT_EQ(metadata.stype, ZES_STRUCTURE_TYPE_INFO_LOG_METADATA_EXT);
    EXPECT_GT(metadata.lengthOfData, 0u);
    EXPECT_LE(static_cast<uint64_t>(metadata.offset) + metadata.lengthOfData,
              records.size);
  }
  std::sort(sorted.begin(), sorted.end(),
            [](const auto &a, const auto &b) { return a.offset < b.offset; });
  for (size_t i = 1; i < sorted.size(); i++) {
    EXPECT_GE(sorted[i].offset, static_cast<uint64_t>(sorted[i - 1].offset) +
                                    sorted[i - 1].lengthOfData)
        << "Records " << i - 1 << " and " << i << " overlap";
  }
}

// The descriptor and payload ranges come from the driver, so they are checked
// before use to report a failure instead of reading outside the vectors.
bool is_record_in_bounds(const InfoLogRecords &records, uint32_t index) {
  if (index >= records.record_count || index >= records.descriptors.size()) {
    return false;
  }
  const auto &metadata = records.descriptors[index];
  const uint64_t end =
      static_cast<uint64_t>(metadata.offset) + metadata.lengthOfData;
  return end <= records.size && end <= records.data.size();
}

bool is_same_record(const InfoLogRecords &first, uint32_t first_index,
                    const InfoLogRecords &second, uint32_t second_index) {
  if (!is_record_in_bounds(first, first_index) ||
      !is_record_in_bounds(second, second_index)) {
    return false;
  }
  const auto &a = first.descriptors[first_index];
  const auto &b = second.descriptors[second_index];
  return a.lengthOfData == b.lengthOfData && a.timestamp == b.timestamp &&
         std::memcmp(first.data.data() + a.offset,
                     second.data.data() + b.offset, a.lengthOfData) == 0;
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenComponentCountZeroWhenRetrievingInfoLogHandlesThenNonZeroCountAndValidHandlesAreReturned) {
  uint32_t count = 0;
  auto handles = lzt::get_info_log_handles(driver, count);
  EXPECT_GT(count, 0u);
  ASSERT_EQ(handles.size(), count);
  for (auto handle : handles) {
    EXPECT_NE(nullptr, handle);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenInvalidComponentCountWhenRetrievingInfoLogHandlesThenActualComponentCountIsUpdated) {
  uint32_t actual_count = lzt::get_info_log_handle_count(driver);
  uint32_t test_count = actual_count + 1;
  lzt::get_info_log_handles(driver, test_count);
  EXPECT_EQ(test_count, actual_count);
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenComponentCountLessThanAvailableWhenRetrievingInfoLogHandlesThenOnlyRequestedHandlesAreReturned) {
  uint32_t actual_count = lzt::get_info_log_handle_count(driver);
  if (actual_count < 2) {
    GTEST_SKIP() << "At least two info log handles are required";
  }
  uint32_t test_count = actual_count - 1;
  std::vector<zes_info_log_handle_t> handles(actual_count, nullptr);
  EXPECT_ZE_RESULT_SUCCESS(
      zesDriverEnumInfoLogsExt(driver, &test_count, handles.data()));
  EXPECT_EQ(test_count, actual_count - 1);
  for (uint32_t i = 0; i < test_count; i++) {
    EXPECT_EQ(handles[i], info_log_handles[i]);
  }
  EXPECT_EQ(nullptr, handles[actual_count - 1]);
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenIdenticalInfoLogHandlesAreReturned) {
  uint32_t count = 0;
  auto handles_initial = lzt::get_info_log_handles(driver, count);
  count = 0;
  auto handles_later = lzt::get_info_log_handles(driver, count);
  EXPECT_EQ(handles_initial, handles_later);
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenMultipleThreadsWhenRetrievingInfoLogHandlesThenIdenticalInfoLogHandlesAreReturned) {
  constexpr uint32_t num_threads = 8;
  std::vector<std::vector<zes_info_log_handle_t>> thread_handles(num_threads);
  std::vector<std::thread> threads;
  for (uint32_t i = 0; i < num_threads; i++) {
    threads.emplace_back([&, i]() {
      uint32_t count = 0;
      thread_handles[i] = lzt::get_info_log_handles(driver, count);
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }
  for (const auto &handles : thread_handles) {
    EXPECT_EQ(handles, info_log_handles);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidInfoLogHandleWhenRetrievingInfoLogPropertiesThenValidPropertiesAreReturned) {
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    EXPECT_EQ(properties.stype, ZES_STRUCTURE_TYPE_INFO_LOG_EXT_PROPERTIES);
    EXPECT_EQ(properties.infoLogType, ZES_INFO_LOG_TYPE_EXT_DEVICE);
    EXPECT_EQ(properties.infoLogFormat, ZES_INFO_LOG_FORMAT_EXT_CPER);
    LOG_INFO << "isNamedInstanceSupported: "
             << static_cast<uint32_t>(properties.isNamedInstanceSupported)
             << " isPeekDataSupported: "
             << static_cast<uint32_t>(properties.isPeekDataSupported);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidInfoLogHandleWhenRetrievingInfoLogPropertiesTwiceThenIdenticalPropertiesAreReturned) {
  for (auto handle : info_log_handles) {
    auto properties_initial = lzt::get_info_log_properties(handle);
    auto properties_later = lzt::get_info_log_properties(handle);
    EXPECT_EQ(properties_initial.infoLogType, properties_later.infoLogType);
    EXPECT_EQ(properties_initial.infoLogFormat, properties_later.infoLogFormat);
    EXPECT_EQ(properties_initial.isNamedInstanceSupported,
              properties_later.isNamedInstanceSupported);
    EXPECT_EQ(properties_initial.isPeekDataSupported,
              properties_later.isPeekDataSupported);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidInfoLogHandleWhenCreatingDefaultInstanceThenInstanceIsCreatedAndDeletedSuccessfully) {
  for (auto handle : info_log_handles) {
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenDeletedDefaultInstanceWhenCreatingDefaultInstanceAgainThenInstanceIsCreatedSuccessfully) {
  constexpr uint32_t iterations = 3;
  for (auto handle : info_log_handles) {
    for (uint32_t i = 0; i < iterations; i++) {
      auto instance = lzt::create_info_log_instance(handle, nullptr);
      ASSERT_NE(nullptr, instance);
      lzt::delete_info_log_instance(instance);
    }
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenZeroBufferSizeWhenCreatingInstanceThenDefaultBufferSizeIsReturned) {
  for (auto handle : info_log_handles) {
    uint32_t buffer_size_in_kb = 0;
    auto instance =
        lzt::create_info_log_instance(handle, nullptr, &buffer_size_in_kb);
    ASSERT_NE(nullptr, instance);
    EXPECT_GT(buffer_size_in_kb, 0u);
    LOG_INFO << "Default buffer size: " << buffer_size_in_kb << " KB";
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenNonZeroBufferSizeWhenCreatingInstanceThenAppliedBufferSizeIsReturned) {
  for (auto handle : info_log_handles) {
    uint32_t buffer_size_in_kb = requested_buffer_size_in_kb;
    auto instance =
        lzt::create_info_log_instance(handle, nullptr, &buffer_size_in_kb);
    ASSERT_NE(nullptr, instance);
    EXPECT_GT(buffer_size_in_kb, 0u);
    LOG_INFO << "Requested buffer size: " << requested_buffer_size_in_kb
             << " KB, applied buffer size: " << buffer_size_in_kb << " KB";
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenNamedInstanceSupportedWhenCreatingNamedInstanceThenInstanceIsCreatedAndDeletedSuccessfully) {
  bool is_named_instance_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isNamedInstanceSupported) {
      continue;
    }
    is_named_instance_supported = true;
    uint32_t buffer_size_in_kb = 0;
    auto instance = lzt::create_info_log_instance(handle, named_instance_name,
                                                  &buffer_size_in_kb);
    ASSERT_NE(nullptr, instance);
    EXPECT_GT(buffer_size_in_kb, 0u);
    lzt::delete_info_log_instance(instance);
  }
  if (!is_named_instance_supported) {
    GTEST_SKIP() << "Named instances are not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenNamedInstanceNotSupportedWhenCreatingNamedInstanceThenUnsupportedFeatureIsReturned) {
  bool is_named_instance_unsupported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (properties.isNamedInstanceSupported) {
      continue;
    }
    is_named_instance_unsupported = true;
    zes_info_log_instance_ext_desc_t desc = {
        ZES_STRUCTURE_TYPE_INFO_LOG_INSTANCE_EXT_DESC, nullptr, nullptr};
    zes_info_log_instance_handle_t instance = nullptr;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
              zesInfoLogCreateInstanceExt(handle, named_instance_name, &desc,
                                          &instance));
  }
  if (!is_named_instance_unsupported) {
    GTEST_SKIP() << "Named instances are supported by all info logs";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenActiveNamedInstanceWhenCreatingInstanceWithSameNameThenHandleObjectInUseIsReturned) {
  bool is_named_instance_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isNamedInstanceSupported) {
      continue;
    }
    is_named_instance_supported = true;
    auto instance = lzt::create_info_log_instance(handle, named_instance_name);
    ASSERT_NE(nullptr, instance);

    zes_info_log_instance_ext_desc_t desc = {
        ZES_STRUCTURE_TYPE_INFO_LOG_INSTANCE_EXT_DESC, nullptr, nullptr};
    zes_info_log_instance_handle_t duplicate_instance = nullptr;
    EXPECT_EQ(ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE,
              zesInfoLogCreateInstanceExt(handle, named_instance_name, &desc,
                                          &duplicate_instance));
    lzt::delete_info_log_instance(instance);
  }
  if (!is_named_instance_supported) {
    GTEST_SKIP() << "Named instances are not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenDeletedNamedInstanceWhenCreatingInstanceWithSameNameThenInstanceIsCreatedSuccessfully) {
  bool is_named_instance_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isNamedInstanceSupported) {
      continue;
    }
    is_named_instance_supported = true;
    auto instance = lzt::create_info_log_instance(handle, named_instance_name);
    ASSERT_NE(nullptr, instance);
    lzt::delete_info_log_instance(instance);

    instance = lzt::create_info_log_instance(handle, named_instance_name);
    ASSERT_NE(nullptr, instance);
    lzt::delete_info_log_instance(instance);
  }
  if (!is_named_instance_supported) {
    GTEST_SKIP() << "Named instances are not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidInstanceWhenQueryingRecordsWithReadThenConsistentSizeAndRecordCountAreReturned) {
  for (auto handle : info_log_handles) {
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    uint32_t size = 0;
    uint32_t record_count = 0;
    zes_info_log_read_status_ext_t read_status = {
        ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT, nullptr};
    ze_result_t result = lzt::read_info_log_records(
        instance, false, read_timeout_ms, size, nullptr, record_count, nullptr,
        &read_status);
    EXPECT_TRUE(lzt::is_info_log_read_successful(result))
        << lzt::to_string(result);
    EXPECT_EQ(size == 0, record_count == 0);
    LOG_INFO << "Records found: " << record_count << ", size: " << size;
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenPeekSupportedWhenQueryingRecordsWithPeekThenConsistentSizeAndRecordCountAreReturned) {
  bool is_peek_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isPeekDataSupported) {
      continue;
    }
    is_peek_supported = true;
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    uint32_t size = 0;
    uint32_t record_count = 0;
    zes_info_log_read_status_ext_t read_status = {
        ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT, nullptr};
    ze_result_t result = lzt::read_info_log_records(
        instance, true, read_timeout_ms, size, nullptr, record_count, nullptr,
        &read_status);
    EXPECT_TRUE(lzt::is_info_log_read_successful(result))
        << lzt::to_string(result);
    EXPECT_EQ(size == 0, record_count == 0);
    lzt::delete_info_log_instance(instance);
  }
  if (!is_peek_supported) {
    GTEST_SKIP() << "Peek is not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenQueryCallWhenReadingOrPeekingRecordsThenBufferAndDescriptorsAreNotWritten) {
  constexpr uint32_t capacity = 16;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    std::vector<bool> peek_modes = {false};
    if (properties.isPeekDataSupported) {
      peek_modes.push_back(true);
    }
    for (bool peek : peek_modes) {
      // A zero record count with a non-zero size is a query call, and so is
      // a zero size with a non-zero record count.
      for (bool zero_record_count : {true, false}) {
        std::vector<uint8_t> buffer(min_read_size, sentinel_byte);
        std::vector<zes_info_log_metadata_ext_t> descriptors(capacity);
        std::memset(descriptors.data(), sentinel_byte,
                    descriptors.size() * sizeof(descriptors[0]));
        for (auto &metadata : descriptors) {
          metadata.stype = ZES_STRUCTURE_TYPE_INFO_LOG_METADATA_EXT;
          metadata.pNext = nullptr;
        }
        auto descriptors_initial = descriptors;
        uint32_t size = zero_record_count ? min_read_size : 0;
        uint32_t record_count = zero_record_count ? 0 : capacity;
        ze_result_t result = lzt::read_info_log_records(
            instance, peek, read_timeout_ms, size, buffer.data(), record_count,
            descriptors.data(), nullptr);
        EXPECT_TRUE(lzt::is_info_log_read_successful(result))
            << lzt::to_string(result);
        EXPECT_TRUE(std::all_of(buffer.begin(), buffer.end(),
                                [](uint8_t b) { return b == sentinel_byte; }))
            << "Buffer written on query call, peek: " << peek;
        EXPECT_EQ(0, std::memcmp(descriptors.data(), descriptors_initial.data(),
                                 descriptors.size() * sizeof(descriptors[0])))
            << "Descriptors written on query call, peek: " << peek;
      }
    }
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenValidInstanceWhenReadingRecordsThenValidRecordsAndMetadataAreReturned) {
  for (auto handle : info_log_handles) {
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    auto records = collect_info_log_records(instance, false, read_timeout_ms);
    validate_info_log_records(records);
    EXPECT_EQ(records.read_status.stype,
              ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT);
    if (records.result == ZE_RESULT_WARNING_DROPPED_DATA) {
      EXPECT_NE(records.read_status.droppedRecordCount, 0);
    }
    LOG_INFO << "Records read: " << records.record_count
             << ", size: " << records.size << ", consumed data size: "
             << records.read_status.consumedDataSize;
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenPeekSupportedWhenPeekingRecordsThenValidRecordsAndMetadataAreReturned) {
  bool is_peek_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isPeekDataSupported) {
      continue;
    }
    is_peek_supported = true;
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    auto records = collect_info_log_records(instance, true, read_timeout_ms);
    validate_info_log_records(records);
    EXPECT_EQ(records.read_status.stype,
              ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT);
    if (records.result == ZE_RESULT_WARNING_DROPPED_DATA) {
      EXPECT_NE(records.read_status.droppedRecordCount, 0);
    }
    lzt::delete_info_log_instance(instance);
  }
  if (!is_peek_supported) {
    GTEST_SKIP() << "Peek is not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenNullReadStatusWhenReadingOrPeekingRecordsThenSuccessIsReturned) {
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    std::vector<bool> peek_modes = {false};
    if (properties.isPeekDataSupported) {
      peek_modes.push_back(true);
    }
    for (bool peek : peek_modes) {
      uint32_t size = 0;
      uint32_t record_count = 0;
      ze_result_t result =
          lzt::read_info_log_records(instance, peek, read_timeout_ms, size,
                                     nullptr, record_count, nullptr, nullptr);
      EXPECT_TRUE(lzt::is_info_log_read_successful(result))
          << lzt::to_string(result) << ", peek: " << peek;

      size = std::max(size, min_read_size);
      record_count = std::max(record_count, min_read_record_count);
      std::vector<uint8_t> buffer(size);
      std::vector<zes_info_log_metadata_ext_t> descriptors(record_count,
                                                           create_metadata());
      result = lzt::read_info_log_records(instance, peek, read_timeout_ms, size,
                                          buffer.data(), record_count,
                                          descriptors.data(), nullptr);
      EXPECT_TRUE(lzt::is_info_log_read_successful(result))
          << lzt::to_string(result) << ", peek: " << peek;
    }
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenZeroTimeoutWhenReadingRecordsThenFunctionReturnsWithoutSearching) {
  // Generous bound, the call is expected to return immediately.
  constexpr auto max_elapsed = std::chrono::milliseconds(500);
  for (auto handle : info_log_handles) {
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    uint32_t size = min_read_size;
    uint32_t record_count = min_read_record_count;
    std::vector<uint8_t> buffer(size);
    std::vector<zes_info_log_metadata_ext_t> descriptors(record_count,
                                                         create_metadata());
    zes_info_log_read_status_ext_t read_status = {
        ZES_STRUCTURE_TYPE_INFO_LOG_READ_STATUS_EXT, nullptr};
    auto start = std::chrono::steady_clock::now();
    ze_result_t result = lzt::read_info_log_records(
        instance, false, 0, size, buffer.data(), record_count,
        descriptors.data(), &read_status);
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_TRUE(lzt::is_info_log_read_successful(result))
        << lzt::to_string(result);
    EXPECT_LT(elapsed, max_elapsed);
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenSingleDescriptorWhenReadingRecordsThenAtMostOneRecordIsReturned) {
  for (auto handle : info_log_handles) {
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    uint32_t query_size = 0;
    uint32_t query_count = 0;
    ze_result_t result =
        lzt::read_info_log_records(instance, false, read_timeout_ms, query_size,
                                   nullptr, query_count, nullptr, nullptr);
    EXPECT_TRUE(lzt::is_info_log_read_successful(result))
        << lzt::to_string(result);

    InfoLogRecords records;
    records.size = std::max(query_size, min_read_size);
    records.record_count = 1;
    records.data.assign(records.size, 0);
    records.descriptors.assign(1, create_metadata());
    records.result = lzt::read_info_log_records(
        instance, false, read_timeout_ms, records.size, records.data.data(),
        records.record_count, records.descriptors.data(), &records.read_status);
    EXPECT_TRUE(lzt::is_info_log_read_successful(records.result))
        << lzt::to_string(records.result);
    EXPECT_LE(records.record_count, 1u);
    validate_info_log_records(records);
    if (query_count > 1) {
      EXPECT_TRUE(records.read_status.hasDataToRead);
    }
    lzt::delete_info_log_instance(instance);
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenPeekedRecordsWhenPeekingAgainThenPreviouslyPeekedRecordsAreStillReturned) {
  bool is_peek_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isPeekDataSupported) {
      continue;
    }
    is_peek_supported = true;
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    auto first = collect_info_log_records(instance, true, UINT64_MAX);
    auto second = collect_info_log_records(instance, true, UINT64_MAX);
    validate_info_log_records(first);
    validate_info_log_records(second);
    if (first.result == ZE_RESULT_WARNING_DROPPED_DATA ||
        second.result == ZE_RESULT_WARNING_DROPPED_DATA) {
      LOG_WARNING << "Records were dropped between peeks, skipping comparison";
    } else {
      EXPECT_GE(second.record_count, first.record_count);
      for (uint32_t i = 0;
           i < std::min(first.record_count, second.record_count); i++) {
        EXPECT_TRUE(is_same_record(first, i, second, i))
            << "Record " << i << " differs between peeks";
      }
    }
    lzt::delete_info_log_instance(instance);
  }
  if (!is_peek_supported) {
    GTEST_SKIP() << "Peek is not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenPeekedRecordsWhenReadingRecordsThenPeekedRecordsAreNotConsumed) {
  bool is_peek_supported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (!properties.isPeekDataSupported) {
      continue;
    }
    is_peek_supported = true;
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    auto peeked = collect_info_log_records(instance, true, UINT64_MAX);
    auto read = collect_info_log_records(instance, false, read_timeout_ms);
    validate_info_log_records(peeked);
    validate_info_log_records(read);
    if (peeked.record_count > 0 &&
        peeked.result != ZE_RESULT_WARNING_DROPPED_DATA &&
        read.result != ZE_RESULT_WARNING_DROPPED_DATA) {
      EXPECT_GT(read.record_count, 0u)
          << "Peeked records were not returned by read";
      if (read.record_count > 0) {
        EXPECT_TRUE(is_same_record(peeked, 0, read, 0))
            << "First record read differs from first record peeked";
      }
    }
    lzt::delete_info_log_instance(instance);
  }
  if (!is_peek_supported) {
    GTEST_SKIP() << "Peek is not supported by any info log";
  }
}

LZT_TEST_F(
    INFO_LOG_TEST,
    GivenPeekNotSupportedWhenPeekingRecordsThenUnsupportedFeatureIsReturned) {
  bool is_peek_unsupported = false;
  for (auto handle : info_log_handles) {
    auto properties = lzt::get_info_log_properties(handle);
    if (properties.isPeekDataSupported) {
      continue;
    }
    is_peek_unsupported = true;
    auto instance = lzt::create_info_log_instance(handle, nullptr);
    ASSERT_NE(nullptr, instance);
    uint32_t size = 0;
    uint32_t record_count = 0;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
              lzt::read_info_log_records(instance, true, read_timeout_ms, size,
                                         nullptr, record_count, nullptr,
                                         nullptr));
    lzt::delete_info_log_instance(instance);
  }
  if (!is_peek_unsupported) {
    GTEST_SKIP() << "Peek is supported by all info logs";
  }
}

} // namespace
