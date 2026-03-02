/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "test_harness/test_harness_cmdlist.hpp"
#include "test_harness/test_harness_event.hpp"
#include "test_harness/test_harness_image.hpp"
#include "test_harness/test_harness_memory.hpp"
#include "gtest/gtest.h"
#include <level_zero/ze_api.h>
#include <numeric>

namespace lzt = level_zero_tests;

namespace {
enum class operation_t {
  append_memory_copy,
  append_memory_copy_region,
  append_image_copy,
  append_image_copy_region,
  append_image_copy_to_memory,
  append_image_copy_from_memory,
  append_memory_fill
};

std::string to_string(operation_t operation) {
  switch (operation) {
  case operation_t::append_memory_copy:
    return "append_memory_copy";
  case operation_t::append_memory_copy_region:
    return "append_memory_copy_region";
  case operation_t::append_image_copy:
    return "append_image_copy";
  case operation_t::append_image_copy_region:
    return "append_image_copy_region";
  case operation_t::append_image_copy_to_memory:
    return "append_image_copy_to_memory";
  case operation_t::append_image_copy_from_memory:
    return "append_image_copy_from_memory";
  case operation_t::append_memory_fill:
    return "append_memory_fill";
  default:
    throw std::runtime_error("Not implemented 'to_string'");
  }
}

static const auto operations = {operation_t::append_memory_copy,
                                operation_t::append_memory_copy_region,
                                operation_t::append_memory_fill,
                                operation_t::append_image_copy,
                                operation_t::append_image_copy_region,
                                operation_t::append_image_copy_to_memory,
                                operation_t::append_image_copy_from_memory};

class EmptyCopySignalAndWaitCheck
    : public ::testing::Test,
      public ::testing::WithParamInterface<operation_t> {
protected:
  operation_t operation = operation_t::append_memory_copy;
  ze_context_handle_t context = nullptr;
  ze_device_handle_t device = nullptr;
  ze_command_list_handle_t command_list = nullptr;
  void SetUp() override {
    context = lzt::get_default_context();
    device = lzt::get_default_device(lzt::get_default_driver());
    command_list = lzt::create_immediate_command_list(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);
    operation = GetParam();
  }
};

LZT_TEST_P(
    EmptyCopySignalAndWaitCheck,
    GivenEmptyCopyWhenSignalEventIsSetWhenSecondCommandIsExecutedThenWorkFinishes) {
  bool is_image_case = [&]() {
    switch (operation) {
    case operation_t::append_image_copy:
    case operation_t::append_image_copy_region:
    case operation_t::append_image_copy_to_memory:
    case operation_t::append_image_copy_from_memory:
      return true;
    default:
      return false;
    }
  }();

  if (is_image_case && !(lzt::image_support())) {
    GTEST_SKIP() << "device does not support images, cannot run test";
  }
  static constexpr size_t mem_size = 512;
  static constexpr int mem_ints = static_cast<int>(mem_size / sizeof(int));
  void *memory = lzt::allocate_shared_memory(mem_size);
  std::iota(static_cast<int *>(memory), static_cast<int *>(memory) + mem_ints,
            0);
  void *output = lzt::allocate_shared_memory(mem_size);
  std::memset(output, 0, mem_size);

  static constexpr uint32_t image_width = 128;
  static constexpr uint32_t image_height = 128;
  static constexpr size_t image_bytes =
      image_width * image_height * sizeof(uint32_t);

  lzt::zeEventPool event_pool;
  event_pool.InitEventPool(2);
  ze_event_handle_t event_signal = nullptr;
  event_pool.create_event(event_signal);
  ze_event_handle_t event_wait = nullptr;
  event_pool.create_event(event_wait);
  switch (operation) {
  case operation_t::append_memory_copy: {
    lzt::append_memory_copy(command_list, output, memory, 0, event_wait, 0,
                            nullptr);
    lzt::append_memory_copy(command_list, output, memory, mem_size,
                            event_signal, 1, &event_wait);
    break;
  }
  case operation_t::append_memory_copy_region: {
    ze_copy_region_t zero_region = {0, 0, 0, 0, 0, 0};
    ze_copy_region_t region = {0, 0, 0, mem_size, 1, 1};
    lzt::append_memory_copy_region(command_list, output, &zero_region, 0, 0,
                                   memory, &zero_region, 0, 0, event_wait, 0,
                                   nullptr);
    lzt::append_memory_copy_region(command_list, output, &region, mem_size, 0,
                                   memory, &region, mem_size, 0, event_signal,
                                   1, &event_wait);
    break;
  }
  case operation_t::append_memory_fill: {
    int pattern = 0x8;
    lzt::append_memory_fill(command_list, output, &pattern, sizeof(pattern), 0,
                            event_wait, 0, nullptr);
    lzt::append_memory_fill(command_list, output, &pattern, sizeof(pattern),
                            mem_size, event_signal, 1, &event_wait);
    break;
  }
  case operation_t::append_image_copy: {
    lzt::zeImageCreateCommon img_common;
    lzt::copy_image_from_mem(img_common.dflt_host_image_,
                             img_common.dflt_device_image_);
    ze_image_region_t zero_img_region = {0, 0, 0, 0, 0, 0};
    lzt::append_image_copy_region(command_list, img_common.dflt_device_image_2_,
                                  img_common.dflt_device_image_,
                                  &zero_img_region, &zero_img_region,
                                  event_wait, 0, nullptr);
    lzt::append_image_copy(command_list, img_common.dflt_device_image_2_,
                           img_common.dflt_device_image_, event_signal, 1,
                           &event_wait);
    lzt::event_host_synchronize(event_signal,
                                std::numeric_limits<uint64_t>::max());
    lzt::ImagePNG32Bit result(image_width, image_height);
    lzt::copy_image_to_mem(img_common.dflt_device_image_2_, result);
    EXPECT_EQ(0, lzt::compare_data_pattern(result, img_common.dflt_host_image_,
                                           0, 0, image_width, image_height, 0,
                                           0, image_width, image_height));
    break;
  }
  case operation_t::append_image_copy_region: {
    lzt::zeImageCreateCommon img_common;
    lzt::copy_image_from_mem(img_common.dflt_host_image_,
                             img_common.dflt_device_image_);
    ze_image_region_t zero_img_region = {0, 0, 0, 0, 0, 0};
    ze_image_region_t full_img_region = {0, 0, 0, image_width, image_height, 1};
    lzt::append_image_copy_region(command_list, img_common.dflt_device_image_2_,
                                  img_common.dflt_device_image_,
                                  &zero_img_region, &zero_img_region,
                                  event_wait, 0, nullptr);
    lzt::append_image_copy_region(command_list, img_common.dflt_device_image_2_,
                                  img_common.dflt_device_image_,
                                  &full_img_region, &full_img_region,
                                  event_signal, 1, &event_wait);
    lzt::event_host_synchronize(event_signal,
                                std::numeric_limits<uint64_t>::max());
    lzt::ImagePNG32Bit result(image_width, image_height);
    lzt::copy_image_to_mem(img_common.dflt_device_image_2_, result);
    EXPECT_EQ(0, lzt::compare_data_pattern(result, img_common.dflt_host_image_,
                                           0, 0, image_width, image_height, 0,
                                           0, image_width, image_height));
    break;
  }
  case operation_t::append_image_copy_from_memory: {
    lzt::zeImageCreateCommon img_common;
    void *src_pixels =
        const_cast<uint32_t *>(img_common.dflt_host_image_.raw_data());
    ze_image_region_t zero_img_region = {0, 0, 0, 0, 0, 0};
    lzt::append_image_copy_from_mem(command_list, img_common.dflt_device_image_,
                                    src_pixels, zero_img_region, event_wait, 0,
                                    nullptr);
    lzt::append_image_copy_from_mem(command_list, img_common.dflt_device_image_,
                                    src_pixels, event_signal, 1, &event_wait);
    lzt::event_host_synchronize(event_signal,
                                std::numeric_limits<uint64_t>::max());
    lzt::ImagePNG32Bit result(image_width, image_height);
    lzt::copy_image_to_mem(img_common.dflt_device_image_, result);
    EXPECT_EQ(0, lzt::compare_data_pattern(result, img_common.dflt_host_image_,
                                           0, 0, image_width, image_height, 0,
                                           0, image_width, image_height));
    break;
  }
  case operation_t::append_image_copy_to_memory: {
    lzt::zeImageCreateCommon img_common;
    lzt::copy_image_from_mem(img_common.dflt_host_image_,
                             img_common.dflt_device_image_);
    void *img_output = lzt::allocate_shared_memory(image_bytes);
    std::memset(img_output, 0, image_bytes);
    ze_image_region_t zero_img_region = {0, 0, 0, 0, 0, 0};
    lzt::append_image_copy_to_mem(command_list, img_output,
                                  img_common.dflt_device_image_,
                                  zero_img_region, event_wait, 0, nullptr);
    lzt::append_image_copy_to_mem(command_list, img_output,
                                  img_common.dflt_device_image_, event_signal,
                                  1, &event_wait);
    lzt::event_host_synchronize(event_signal,
                                std::numeric_limits<uint64_t>::max());
    const uint32_t *expected = img_common.dflt_host_image_.raw_data();
    const uint32_t *actual = static_cast<const uint32_t *>(img_output);
    int pixel_errors = 0;
    for (size_t i = 0; i < image_width * image_height; ++i) {
      if (expected[i] != actual[i])
        ++pixel_errors;
    }
    EXPECT_EQ(0, pixel_errors);
    lzt::free_memory(img_output);
    break;
  }
  }

  if (!is_image_case) {
    lzt::event_host_synchronize(event_signal,
                                std::numeric_limits<uint64_t>::max());

    switch (operation) {
    case operation_t::append_memory_copy:
    case operation_t::append_memory_copy_region:
      EXPECT_EQ(0, std::memcmp(output, memory, mem_size));
      break;
    case operation_t::append_memory_fill: {
      const int expected_int = 0x08080808;
      const int *out_ints = static_cast<const int *>(output);
      int fill_errors = 0;
      for (int i = 0; i < mem_ints; ++i) {
        if (out_ints[i] != expected_int)
          ++fill_errors;
      }
      EXPECT_EQ(0, fill_errors);
      break;
    }
    default:
      break;
    }
  }

  lzt::free_memory(memory);
  lzt::free_memory(output);
}

INSTANTIATE_TEST_SUITE_P(EmptyCopySignalAndWaitCheckParam,
                         EmptyCopySignalAndWaitCheck,
                         ::testing::ValuesIn(operations));

} // namespace
