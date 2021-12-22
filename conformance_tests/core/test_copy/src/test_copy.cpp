/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

using namespace level_zero_tests;

namespace {

class zeCommandListAppendMemoryFillTests : public ::testing::Test {};

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillThenSuccessIsReturned) {
  lzt::zeCommandList cl;
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  const int pattern_size = 1;

  lzt::append_memory_fill(cl.command_list_, memory, &pattern, pattern_size,
                          size, nullptr);
  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  lzt::zeCommandList cl;
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  ep.create_event(hEvent);
  lzt::append_memory_fill(cl.command_list_, memory, &pattern, pattern_size,
                          size, hEvent);
  ep.destroy_event(hEvent);

  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryFillTests,
    GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  lzt::zeCommandList cl;
  const size_t size = 4096;
  void *memory = allocate_device_memory(size);
  const uint8_t pattern = 0x00;
  ze_event_handle_t hEvent = nullptr;
  const int pattern_size = 1;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  lzt::append_memory_fill(cl.command_list_, memory, &pattern, pattern_size,
                          size, nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  ep.destroy_event(hEvent);

  free_memory(memory);
}

class zeCommandListAppendMemoryFillVerificationTests : public ::testing::Test {
protected:
  zeCommandListAppendMemoryFillVerificationTests() {
    command_list = create_command_list();
    cq = create_command_queue();
  }

  ~zeCommandListAppendMemoryFillVerificationTests() {
    destroy_command_queue(cq);
    destroy_command_list(command_list);
  }
  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t cq;
};

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenHostMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {

  size_t size = 16;
  auto memory = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;

  append_memory_fill(command_list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(command_list, nullptr, 0, nullptr);
  close_command_list(command_list);
  execute_command_lists(cq, 1, &command_list, nullptr);
  synchronize(cq, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  free_memory(memory);
}

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenSharedMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {

  size_t size = 16;
  auto memory = allocate_shared_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;

  append_memory_fill(command_list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(command_list, nullptr, 0, nullptr);
  close_command_list(command_list);
  execute_command_lists(cq, 1, &command_list, nullptr);
  synchronize(cq, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(memory)[i], pattern)
        << "Memory Fill did not match.";
  }

  free_memory(memory);
}

TEST_F(zeCommandListAppendMemoryFillVerificationTests,
       GivenDeviceMemoryWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {

  size_t size = 16;
  auto memory = allocate_device_memory(size);
  auto local_mem = allocate_host_memory(size);
  uint8_t pattern = 0xAB;
  const int pattern_size = 1;

  append_memory_fill(command_list, memory, &pattern, pattern_size, size,
                     nullptr);
  append_barrier(command_list, nullptr, 0, nullptr);
  close_command_list(command_list);
  execute_command_lists(cq, 1, &command_list, nullptr);
  synchronize(cq, UINT64_MAX);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(command_list));
  append_memory_copy(command_list, local_mem, memory, size, nullptr);
  append_barrier(command_list, nullptr, 0, nullptr);
  close_command_list(command_list);
  execute_command_lists(cq, 1, &command_list, nullptr);
  synchronize(cq, UINT64_MAX);

  for (uint32_t i = 0; i < size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(local_mem)[i], pattern)
        << "Memory Fill did not match.";
  }

  free_memory(memory);
  free_memory(local_mem);
}

class zeCommandListAppendMemoryFillSubDeviceVerificationTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_memory_type_t> {};
TEST_P(
    zeCommandListAppendMemoryFillSubDeviceVerificationTests,
    GivenMemoryAllocationWhenExecutingAMemoryFillWithSubDeviceThenMemoryIsSetCorrectly) {

  auto memory_type = GetParam();
  uint32_t test_count = 0;
  auto context = lzt::get_default_context();
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  const size_t size = 16;
  void *host_memory = lzt::allocate_host_memory(size, 1, context);

  for (auto device : devices) {
    auto subdevices = lzt::get_ze_sub_devices(device);

    if (subdevices.empty()) {
      continue;
    }
    test_count++;
    for (auto subdevice : subdevices) {
      auto command_queue = lzt::create_command_queue(subdevice);
      auto command_list = lzt::create_command_list(subdevice);

      void *memory = nullptr;
      switch (memory_type) {
      case ZE_MEMORY_TYPE_DEVICE:
        memory = lzt::allocate_device_memory(size, 1, 0, device, context);
        break;
      case ZE_MEMORY_TYPE_HOST:
        memory = lzt::allocate_host_memory(size, 1, context);
        break;
      case ZE_MEMORY_TYPE_SHARED:
        memory = lzt::allocate_shared_memory(size, 1, 0, 0, device, context);
        break;
      default:
        LOG_WARNING << "Unhandled memory type for memory fill subdevice test: "
                    << memory_type;
      }

      uint8_t pattern = 0xAB;
      const int pattern_size = 1;

      lzt::append_memory_fill(command_list, memory, &pattern, pattern_size,
                              size, nullptr);
      lzt::append_memory_copy(command_list, host_memory, memory, size);
      lzt::close_command_list(command_list);
      lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
      lzt::synchronize(command_queue, UINT64_MAX);

      for (int i = 0; i < size; i++) {
        ASSERT_EQ(static_cast<uint8_t *>(host_memory)[i], pattern)
            << "Memory Fill did not match on sub-device";
      }

      lzt::free_memory(memory);
      lzt::destroy_command_list(command_list);
      lzt::destroy_command_queue(command_queue);
    }
  }
  lzt::free_memory(host_memory);

  if (!test_count) {
    LOG_WARNING << "No Sub-Device Memory Fill tests run";
  }
}

INSTANTIATE_TEST_CASE_P(SubDeviceMemoryFills,
                        zeCommandListAppendMemoryFillSubDeviceVerificationTests,
                        ::testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                          ZE_MEMORY_TYPE_HOST,
                                          ZE_MEMORY_TYPE_SHARED));

class zeCommandListAppendMemoryFillPatternVerificationTests
    : public zeCommandListAppendMemoryFillVerificationTests,
      public ::testing::WithParamInterface<size_t> {};

TEST_P(zeCommandListAppendMemoryFillPatternVerificationTests,
       GivenPatternSizeWhenExecutingAMemoryFillThenMemoryIsSetCorrectly) {

  const int pattern_size = GetParam();
  const size_t total_size = (pattern_size * 10) + 5;
  std::unique_ptr<uint8_t> pattern(new uint8_t[pattern_size]);
  auto target_memory = allocate_host_memory(total_size);

  for (uint32_t i = 0; i < pattern_size; i++) {
    pattern.get()[i] = i;
  }

  append_memory_fill(command_list, target_memory, pattern.get(), pattern_size,
                     total_size, nullptr);
  append_barrier(command_list, nullptr, 0, nullptr);
  close_command_list(command_list);
  execute_command_lists(cq, 1, &command_list, nullptr);
  synchronize(cq, UINT64_MAX);

  for (uint32_t i = 0; i < total_size; i++) {
    ASSERT_EQ(static_cast<uint8_t *>(target_memory)[i], i % pattern_size)
        << "Memory Fill did not match.";
  }
  free_memory(target_memory);
}

INSTANTIATE_TEST_CASE_P(VaryPatternSize,
                        zeCommandListAppendMemoryFillPatternVerificationTests,
                        ::testing::Values(1, 2, 4, 8, 16, 32, 64, 128));

class zeCommandListCommandQueueTests : public ::testing::Test {
protected:
  zeCommandList cl;
  zeCommandQueue cq;
};

class zeCommandListAppendMemoryCopyWithDataVerificationTests
    : public zeCommandListCommandQueueTests {};

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  const size_t size = 4 * 1024;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(size_in_bytes(host_memory1));

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  append_memory_copy(cl.command_list_, device_memory, host_memory1.data(),
                     size_in_bytes(host_memory1), nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  append_memory_copy(cl.command_list_, host_memory2.data(), device_memory,
                     size_in_bytes(host_memory2), nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  close_command_list(cl.command_list_);
  execute_command_lists(cq.command_queue_, 1, &cl.command_list_, nullptr);
  synchronize(cq.command_queue_, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);
  free_memory(device_memory);
}

TEST_F(
    zeCommandListAppendMemoryCopyWithDataVerificationTests,
    GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  const size_t size = 1024;
  // number of transfers to device memory
  const size_t num_dev_mem_copy = 8;
  // byte address alignment
  const size_t addr_alignment = 1;
  std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
  void *device_memory = allocate_device_memory(
      num_dev_mem_copy * (size_in_bytes(host_memory1) + addr_alignment));

  lzt::write_data_pattern(host_memory1.data(), size, 1);

  uint8_t *char_src_ptr = static_cast<uint8_t *>(device_memory);
  uint8_t *char_dst_ptr = char_src_ptr;
  append_memory_copy(cl.command_list_, static_cast<void *>(char_dst_ptr),
                     host_memory1.data(), size, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  for (uint32_t i = 1; i < num_dev_mem_copy; i++) {
    char_src_ptr = char_dst_ptr;
    char_dst_ptr += (size + addr_alignment);
    append_memory_copy(cl.command_list_, static_cast<void *>(char_dst_ptr),
                       static_cast<void *>(char_src_ptr), size, nullptr);
    append_barrier(cl.command_list_, nullptr, 0, nullptr);
  }
  append_memory_copy(cl.command_list_, host_memory2.data(),
                     static_cast<void *>(char_dst_ptr), size, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  close_command_list(cl.command_list_);
  execute_command_lists(cq.command_queue_, 1, &cl.command_list_, nullptr);
  synchronize(cq.command_queue_, UINT64_MAX);

  lzt::validate_data_pattern(host_memory2.data(), size, 1);

  free_memory(device_memory);
}

class zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests
    : public zeCommandListCommandQueueTests {};

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndSharedMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyThenSuccessIsReturnedAndCopyIsCorrect) {
  const size_t size = 4 * 1024;
  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();

  EXPECT_NE(src_context, dflt_context);

  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);
  void *shared_memory_src_ctx = lzt::allocate_shared_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *shared_memory_dflt_ctx = lzt::allocate_shared_memory(size);
  void *device_memory_src_ctx = lzt::allocate_device_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *device_memory_dflt_ctx = lzt::allocate_device_memory(size);

  lzt::write_data_pattern(host_memory_src_ctx, size, 1);

  append_memory_copy(src_context, cl.command_list_, device_memory_dflt_ctx,
                     host_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  append_memory_copy(dflt_context, cl.command_list_, device_memory_src_ctx,
                     device_memory_dflt_ctx, size, nullptr, 0, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  append_memory_copy(src_context, cl.command_list_, shared_memory_dflt_ctx,
                     device_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  append_memory_copy(dflt_context, cl.command_list_, shared_memory_src_ctx,
                     device_memory_dflt_ctx, size, nullptr, 0, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  append_memory_copy(src_context, cl.command_list_, host_memory_dflt_ctx,
                     shared_memory_src_ctx, size, nullptr, 0, nullptr);
  append_barrier(cl.command_list_, nullptr, 0, nullptr);
  close_command_list(cl.command_list_);
  execute_command_lists(cq.command_queue_, 1, &cl.command_list_, nullptr);
  synchronize(cq.command_queue_, UINT64_MAX);

  lzt::validate_data_pattern(host_memory_dflt_ctx, size, 1);
  free_memory(src_context, host_memory_src_ctx);
  free_memory(host_memory_dflt_ctx);
  free_memory(src_context, shared_memory_src_ctx);
  free_memory(shared_memory_dflt_ctx);
  free_memory(src_context, device_memory_src_ctx);
  free_memory(device_memory_dflt_ctx);
}

TEST_F(
    zeCommandListAppendMemoryCopyFromContextWithDataVerificationTests,
    GivenHostMemoryAndDeviceMemoryFromTwoContextsWhenAppendingMemoryCopyWithEventsThenSuccessIsReturnedAndCopyIsCorrect) {
  const size_t size = 4 * 1024;
  ze_context_handle_t src_context = lzt::create_context();
  ze_context_handle_t dflt_context = lzt::get_default_context();
  EXPECT_NE(src_context, dflt_context);
  ze_event_handle_t hEvent1 = nullptr;
  ze_event_handle_t hEvent2 = nullptr;
  lzt::zeEventPool ep;
  ep.create_event(hEvent1);
  ep.create_event(hEvent2);

  void *host_memory_src_ctx = lzt::allocate_host_memory(size, 1, src_context);
  void *host_memory_dflt_ctx = lzt::allocate_host_memory(size);
  void *device_memory_src_ctx = lzt::allocate_device_memory(
      size, 1, 0, 0, zeDevice::get_instance()->get_device(), src_context);
  void *device_memory_dflt_ctx = lzt::allocate_device_memory(size);

  lzt::write_data_pattern(host_memory_src_ctx, size, 1);

  append_memory_copy(src_context, cl.command_list_, device_memory_dflt_ctx,
                     host_memory_src_ctx, size, hEvent1, 0, nullptr);
  append_barrier(cl.command_list_, hEvent2, 1, &hEvent1);
  append_memory_copy(dflt_context, cl.command_list_, device_memory_src_ctx,
                     device_memory_dflt_ctx, size, hEvent1, 1, &hEvent2);
  append_barrier(cl.command_list_, hEvent2, 1, &hEvent1);
  append_memory_copy(src_context, cl.command_list_, host_memory_dflt_ctx,
                     device_memory_src_ctx, size, hEvent1, 0, &hEvent2);
  append_barrier(cl.command_list_, nullptr, 1, &hEvent1);
  close_command_list(cl.command_list_);
  execute_command_lists(cq.command_queue_, 1, &cl.command_list_, nullptr);
  synchronize(cq.command_queue_, UINT64_MAX);

  lzt::validate_data_pattern(host_memory_dflt_ctx, size, 1);
  ep.destroy_event(hEvent1);
  ep.destroy_event(hEvent2);
  free_memory(src_context, host_memory_src_ctx);
  free_memory(host_memory_dflt_ctx);
  free_memory(src_context, device_memory_src_ctx);
  free_memory(device_memory_dflt_ctx);
}

class zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<std::tuple<
          size_t, size_t, size_t, ze_memory_type_t, ze_memory_type_t>> {
protected:
  zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests() {
    command_list = create_command_list();
    command_queue = create_command_queue();

    rows = std::get<0>(GetParam());
    columns = std::get<1>(GetParam());
    slices = std::get<2>(GetParam());

    memory_size = rows * columns * slices;

    ze_memory_type_t source_type = std::get<3>(GetParam());
    ze_memory_type_t destination_type = std::get<4>(GetParam());

    switch (source_type) {
    case ZE_MEMORY_TYPE_DEVICE:
      source_memory = allocate_device_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_HOST:
      source_memory = allocate_host_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_SHARED:
      source_memory = allocate_shared_memory(memory_size);
      break;
    default:
      abort();
    }

    switch (destination_type) {
    case ZE_MEMORY_TYPE_DEVICE:
      destination_memory = allocate_device_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_HOST:
      destination_memory = allocate_host_memory(memory_size);
      break;
    case ZE_MEMORY_TYPE_SHARED:
      destination_memory = allocate_shared_memory(memory_size);
      break;
    default:
      abort();
    }

    // Set up memory buffers for test
    // Device memory has to be copied in so
    // use temporary buffers
    temp_src = allocate_host_memory(memory_size);
    temp_dest = allocate_host_memory(memory_size);

    memset(temp_dest, 0, memory_size);
    write_data_pattern(temp_src, memory_size, 1);

    append_memory_copy(command_list, destination_memory, temp_dest,
                       memory_size);
    append_memory_copy(command_list, source_memory, temp_src, memory_size);
    append_barrier(command_list);
  }

  ~zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests() {
    destroy_command_list(command_list);
    destroy_command_queue(command_queue);

    free_memory(source_memory);
    free_memory(destination_memory);
    free_memory(temp_src);
    free_memory(temp_dest);
  }

  void test_copy_region() {
    void *verification_memory = allocate_host_memory(memory_size);

    std::array<size_t, 3> widths = {1, columns / 2, columns};
    std::array<size_t, 3> heights = {1, rows / 2, rows};
    std::array<size_t, 3> depths = {1, slices / 2, slices};

    for (int region = 0; region < 3; region++) {
      // Define region to be copied from/to
      auto width = widths[region];
      auto height = heights[region];
      auto depth = depths[region];

      ze_copy_region_t src_region;
      src_region.originX = 0;
      src_region.originY = 0;
      src_region.originZ = 0;
      src_region.width = width;
      src_region.height = height;
      src_region.depth = depth;

      ze_copy_region_t dest_region;
      dest_region.originX = 0;
      dest_region.originY = 0;
      dest_region.originZ = 0;
      dest_region.width = width;
      dest_region.height = height;
      dest_region.depth = depth;

      append_memory_copy_region(command_list, destination_memory, &dest_region,
                                columns, columns * rows, source_memory,
                                &src_region, columns, columns * rows, nullptr);
      append_barrier(command_list);
      append_memory_copy(command_list, verification_memory, destination_memory,
                         memory_size);
      close_command_list(command_list);
      execute_command_lists(command_queue, 1, &command_list, nullptr);
      synchronize(command_queue, UINT64_MAX);
      reset_command_list(command_list);

      for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
            // index calculated based on memory sized by rows * columns * slices
            size_t index = z * columns * rows + y * columns + x;
            uint8_t dest_val =
                static_cast<uint8_t *>(verification_memory)[index];
            uint8_t src_val = static_cast<uint8_t *>(temp_src)[index];

            ASSERT_EQ(dest_val, src_val)
                << "Copy failed with region(w,h,d)=(" << width << ", " << height
                << ", " << depth << ")";
          }
        }
      }
    }
  }

  ze_command_list_handle_t command_list;
  ze_command_queue_handle_t command_queue;

  void *source_memory, *temp_src;
  void *destination_memory, *temp_dest;

  size_t rows;
  size_t columns;
  size_t slices;
  size_t memory_size;
};

TEST_P(
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests,
    GivenValidMemoryWhenAppendingMemoryCopyWithRegionThenSuccessIsReturnedAndCopyIsCorrect) {
  test_copy_region();
}

auto memory_types = ::testing::Values(
    ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_HOST, ZE_MEMORY_TYPE_SHARED);
INSTANTIATE_TEST_CASE_P(
    MemoryCopies,
    zeCommandListAppendMemoryCopyRegionWithDataVerificationParameterizedTests,
    ::testing::Combine(::testing::Values(8, 64),    // Rows
                       ::testing::Values(8, 64),    // Cols
                       ::testing::Values(1, 8, 64), // Slices
                       memory_types,                // Source Memory Type
                       memory_types                 // Destination Memory Type
                       ));
class zeCommandListAppendMemoryCopyTests : public ::testing::Test {};

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyThenSuccessIsReturned) {
  lzt::zeCommandList cl;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = allocate_device_memory(size_in_bytes(host_memory));

  append_memory_copy(cl.command_list_, memory, host_memory.data(),
                     size_in_bytes(host_memory), nullptr);

  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  lzt::zeCommandList cl;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = allocate_device_memory(size_in_bytes(host_memory));
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  append_memory_copy(cl.command_list_, memory, host_memory.data(),
                     size_in_bytes(host_memory), hEvent);

  ep.destroy_event(hEvent);

  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  lzt::zeCommandList cl;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = allocate_device_memory(size_in_bytes(host_memory));
  ze_event_handle_t hEvent = nullptr;

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  append_memory_copy(cl.command_list_, memory, host_memory.data(),
                     size_in_bytes(host_memory), nullptr, 1, &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  ep.destroy_event(hEvent);

  free_memory(memory);
}

TEST_F(
    zeCommandListAppendMemoryCopyTests,
    GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventThenSuccessIsReturned) {
  lzt::zeEventPool ep;
  lzt::zeCommandList cl;
  const size_t size = 16;
  const std::vector<char> host_memory(size, 123);
  void *memory = allocate_device_memory(size_in_bytes(host_memory));
  ze_event_handle_t hEvent = nullptr;
  ze_copy_region_t dstRegion = {};
  ze_copy_region_t srcRegion = {};

  ep.create_event(hEvent);
  auto hEvent_before = hEvent;
  append_memory_copy_region(cl.command_list_, memory, &dstRegion, 0, 0,
                            host_memory.data(), &srcRegion, 0, 0, nullptr, 1,
                            &hEvent);
  ASSERT_EQ(hEvent, hEvent_before);
  ep.destroy_event(hEvent);

  free_memory(memory);
}

TEST(zeCommandListAppendMemoryPrefetchTests,
     GivenMemoryPrefetchWhenWritingFromDeviceThenDataisCorrectFromHost) {
  const size_t size = 16;
  const uint8_t value = 0x55;
  auto command_list = lzt::create_command_list();
  void *memory = allocate_shared_memory(size);
  memset(memory, 0xaa, size);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemoryPrefetch(command_list, memory, size));
  lzt::append_memory_set(command_list, memory, &value, size);
  auto command_queue = lzt::create_command_queue();
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(value, ((uint8_t *)memory)[i]);
  }

  free_memory(memory);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

class zeCommandListAppendMemAdviseTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<ze_memory_advice_t> {};

TEST_P(zeCommandListAppendMemAdviseTests,
       GivenMemAdviseWhenWritingFromDeviceThenDataIsCorrectFromHost) {
  const size_t size = 16;
  const uint8_t value = 0x55;
  auto command_list = lzt::create_command_list();
  void *memory = allocate_shared_memory(size);
  memset(memory, 0xaa, size);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeCommandListAppendMemAdvise(command_list,
                                         zeDevice::get_instance()->get_device(),
                                         memory, size, GetParam()));

  lzt::append_memory_set(command_list, memory, &value, size);
  auto command_queue = lzt::create_command_queue();
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
  lzt::synchronize(command_queue, UINT64_MAX);

  for (size_t i = 0; i < size; i++) {
    ASSERT_EQ(value, ((uint8_t *)memory)[i]);
  }

  free_memory(memory);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

INSTANTIATE_TEST_CASE_P(
    MemAdviceFlags, zeCommandListAppendMemAdviseTests,
    ::testing::Values(ZE_MEMORY_ADVICE_SET_READ_MOSTLY,
                      ZE_MEMORY_ADVICE_CLEAR_READ_MOSTLY,
                      ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION,
                      ZE_MEMORY_ADVICE_CLEAR_PREFERRED_LOCATION,
                      ZE_MEMORY_ADVICE_SET_NON_ATOMIC_MOSTLY,
                      ZE_MEMORY_ADVICE_CLEAR_NON_ATOMIC_MOSTLY,
                      ZE_MEMORY_ADVICE_BIAS_CACHED,
                      ZE_MEMORY_ADVICE_BIAS_UNCACHED));

} // namespace