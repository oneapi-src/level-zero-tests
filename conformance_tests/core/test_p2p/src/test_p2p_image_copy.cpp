/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class P2PImageCopy : public ::testing::Test {
protected:
  void SetUp() override {

    auto devices = lzt::get_ze_devices(lzt::get_default_driver());
    if (devices.size() < 2) {
      skip = true;
      GTEST_SKIP() << "WARNING:  Less than 2 devices found, cannot run test";
      return;
    }
    if (!lzt::can_access_peer(devices[0], devices[1])) {
      skip = true;
      GTEST_SKIP()
          << "dev0 and dev1 fail zeDeviceCanAccessPeer check, cannot run test";
      return;
    }

    dev0 = devices[0];
    dev1 = devices[1];
    auto img_prop = lzt::get_image_properties(dev0);
    if (!img_prop.maxReadImageArgs || !img_prop.maxWriteImageArgs) {
      skip = true;
      GTEST_SKIP() << "device does not support images, cannot run test";
      return;
    }

    img_prop = lzt::get_image_properties(dev1);
    if (!img_prop.maxReadImageArgs || !img_prop.maxWriteImageArgs) {
      skip = true;
      GTEST_SKIP() << "device does not support images, cannot run test";
      return;
    }

    input_png = lzt::ImagePNG32Bit("test_input.png");
    img_width = input_png.width();
    img_height = input_png.height();
    output_png = lzt::ImagePNG32Bit(img_width, img_height);

    ze_image_desc_t img_desc = {};
    img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    img_desc.flags = 0;
    img_desc.type = ZE_IMAGE_TYPE_2D;
    img_desc.format = {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                       ZE_IMAGE_FORMAT_TYPE_UNORM,
                       ZE_IMAGE_FORMAT_SWIZZLE_R,
                       ZE_IMAGE_FORMAT_SWIZZLE_G,
                       ZE_IMAGE_FORMAT_SWIZZLE_B,
                       ZE_IMAGE_FORMAT_SWIZZLE_A},
    img_desc.width = img_width;
    img_desc.height = img_height;
    img_desc.depth = 1;
    img_desc.arraylevels = 0;
    img_desc.miplevels = 0;

    img_dev0 = lzt::create_ze_image(dev0, img_desc);
    img_dev1 = lzt::create_ze_image(dev1, img_desc);

    command_list_dev0 = lzt::create_command_list(dev0);
    command_list_dev1 = lzt::create_command_list(dev1);
    command_q_dev0 = lzt::create_command_queue(dev0);
    command_q_dev1 = lzt::create_command_queue(dev1);

    ze_event_pool_desc_t ep_desc = {};
    ep_desc.stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC;
    ep_desc.flags = 0;
    ep_desc.count = 10;
    ep = lzt::create_event_pool(lzt::get_default_context(), ep_desc, devices);
  }

  void TearDown() override {
    if (skip)
      return;
    lzt::destroy_ze_image(img_dev0);
    lzt::destroy_ze_image(img_dev1);
    lzt::destroy_command_list(command_list_dev0);
    lzt::destroy_command_list(command_list_dev1);
    lzt::destroy_command_queue(command_q_dev0);
    lzt::destroy_command_queue(command_q_dev1);
    lzt::destroy_event_pool(ep);
  }

  lzt::ImagePNG32Bit input_png;
  lzt::ImagePNG32Bit output_png;
  uint32_t img_width;
  uint32_t img_height;
  ze_device_handle_t dev0, dev1;
  ze_image_handle_t img_dev0, img_dev1;
  ze_command_list_handle_t command_list_dev0;
  ze_command_list_handle_t command_list_dev1;
  ze_command_queue_handle_t command_q_dev0;
  ze_command_queue_handle_t command_q_dev1;
  ze_event_pool_handle_t ep;
  bool skip = false;
};

TEST_F(
    P2PImageCopy,
    GivenTwoDevicesAndImageOnDeviceWhenCopiedToOtherDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 1;
  auto event2 = lzt::create_event(ep, event_desc);

  // Load image to dev0
  lzt::append_image_copy_from_mem(command_list_dev0, img_dev0,
                                  input_png.raw_data(), event1);
  lzt::append_wait_on_events(command_list_dev0, 1, &event1);

  // copy to dev 1
  lzt::append_image_copy(command_list_dev0, img_dev1, img_dev0, event2);
  lzt::append_wait_on_events(command_list_dev0, 1, &event2);
  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // Copyback to host
  lzt::append_image_copy_to_mem(command_list_dev1, output_png.raw_data(),
                                img_dev1, nullptr);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
}

TEST_F(
    P2PImageCopy,
    GivenTwoDevicesAndImageOnDeviceWhenRegionCopiedToOtherDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 1;
  auto event2 = lzt::create_event(ep, event_desc);
  event_desc.index = 2;
  auto event3 = lzt::create_event(ep, event_desc);

  // Load image to dev0
  lzt::append_image_copy_from_mem(command_list_dev0, img_dev0,
                                  input_png.raw_data(), event1);
  lzt::append_wait_on_events(command_list_dev0, 1, &event1);

  // copy to dev 1
  ze_image_region_t region = {0, 0, 0, img_width, img_height / 2, 1};
  lzt::append_image_copy_region(command_list_dev0, img_dev1, img_dev0, &region,
                                &region, event2);
  lzt::append_wait_on_events(command_list_dev0, 1, &event2);
  region = {0, img_height / 2, 0, img_width, img_height / 2, 1};
  lzt::append_image_copy_region(command_list_dev0, img_dev1, img_dev0, &region,
                                &region, event3);
  lzt::append_wait_on_events(command_list_dev0, 1, &event3);

  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // Copyback to host
  lzt::append_image_copy_to_mem(command_list_dev1, output_png.raw_data(),
                                img_dev1, nullptr);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
  lzt::destroy_event(event3);
}

class P2PImageCopyMemory
    : public P2PImageCopy,
      public ::testing::WithParamInterface<ze_memory_type_t> {};

TEST_P(
    P2PImageCopyMemory,
    GivenTwoDevicesAndImageOnDeviceWhenCopiedToMemoryOnOtherDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 1;
  auto event2 = lzt::create_event(ep, event_desc);

  void *target_mem;
  size_t mem_size = img_height * img_width * sizeof(uint32_t);
  if (GetParam() == ZE_MEMORY_TYPE_DEVICE) {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem = lzt::allocate_device_memory(mem_size, 1, d_flags, dev1,
                                             lzt::get_default_context());
  } else {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem =
        lzt::allocate_shared_memory(mem_size, 1, d_flags, h_flags, dev1);
  }

  // Load image to dev0
  lzt::append_image_copy_from_mem(command_list_dev0, img_dev0,
                                  input_png.raw_data(), event1);
  lzt::append_wait_on_events(command_list_dev0, 1, &event1);

  // on dev0, copy dev0 image to dev1 memory
  lzt::append_image_copy_to_mem(command_list_dev0, target_mem, img_dev0,
                                event2);
  lzt::append_wait_on_events(command_list_dev0, 1, &event2);
  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // Copyback to host
  lzt::append_memory_copy(command_list_dev1, output_png.raw_data(), target_mem,
                          mem_size);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
  lzt::free_memory(target_mem);
}

TEST_P(
    P2PImageCopyMemory,
    GivenTwoDevicesAndImageOnDeviceWhenCopiedFromMemoryOnOtherDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event = lzt::create_event(ep, event_desc);

  void *target_mem;
  size_t mem_size = img_height * img_width * sizeof(uint32_t);
  if (GetParam() == ZE_MEMORY_TYPE_DEVICE) {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem = lzt::allocate_device_memory(mem_size, 1, d_flags, dev1,
                                             lzt::get_default_context());
  } else {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem =
        lzt::allocate_shared_memory(mem_size, 1, d_flags, h_flags, dev1);
  }

  // on dev1, load image to dev1 memory
  lzt::append_memory_copy(command_list_dev1, target_mem, input_png.raw_data(),
                          mem_size);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // on dev0, copy from dev1 memory to dev0 image
  lzt::append_image_copy_from_mem(command_list_dev0, img_dev0, target_mem,
                                  event);
  lzt::append_wait_on_events(command_list_dev0, 1, &event);

  // Copyback to host
  lzt::append_image_copy_to_mem(command_list_dev0, output_png.raw_data(),
                                img_dev0, nullptr);
  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event);
  lzt::free_memory(target_mem);
}

TEST_P(
    P2PImageCopyMemory,
    GivenTwoDevicesAndImageOnRemoteDeviceWhenCopiedToMemoryOnLocalDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event = lzt::create_event(ep, event_desc);

  void *target_mem;
  size_t mem_size = img_height * img_width * sizeof(uint32_t);
  if (GetParam() == ZE_MEMORY_TYPE_DEVICE) {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem = lzt::allocate_device_memory(mem_size, 1, d_flags, dev1,
                                             lzt::get_default_context());
  } else {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem =
        lzt::allocate_shared_memory(mem_size, 1, d_flags, h_flags, dev1);
  }

  // Load image to dev0
  lzt::append_image_copy_from_mem(command_list_dev0, img_dev0,
                                  input_png.raw_data(), nullptr);
  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // on dev1, copy dev0 image to dev1 memory
  lzt::append_image_copy_to_mem(command_list_dev1, target_mem, img_dev0, event);
  lzt::append_wait_on_events(command_list_dev1, 1, &event);

  // Copyback to host
  lzt::append_memory_copy(command_list_dev1, output_png.raw_data(), target_mem,
                          mem_size);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event);
  lzt::free_memory(target_mem);
}

TEST_P(
    P2PImageCopyMemory,
    GivenTwoDevicesAndImageOnRemoteDeviceWhenCopiedFromMemoryOnLocalDeviceThenResultIsCorrect) {
  if (skip)
    return;

  ze_event_desc_t event_desc = {};
  event_desc.stype = ZE_STRUCTURE_TYPE_EVENT_DESC;
  event_desc.index = 0;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  auto event1 = lzt::create_event(ep, event_desc);
  event_desc.index = 1;
  auto event2 = lzt::create_event(ep, event_desc);

  void *target_mem;
  size_t mem_size = img_height * img_width * sizeof(uint32_t);
  if (GetParam() == ZE_MEMORY_TYPE_DEVICE) {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem = lzt::allocate_device_memory(mem_size, 1, d_flags, dev1,
                                             lzt::get_default_context());
  } else {
    ze_device_mem_alloc_flags_t d_flags = 0;
    ze_host_mem_alloc_flags_t h_flags = 0;
    target_mem =
        lzt::allocate_shared_memory(mem_size, 1, d_flags, h_flags, dev1);
  }

  // on dev1, load image to dev1 memory
  lzt::append_memory_copy(command_list_dev1, target_mem, input_png.raw_data(),
                          mem_size, event1);
  lzt::append_wait_on_events(command_list_dev1, 1, &event1);

  // on dev1, copy from dev1 memory to dev0 image
  lzt::append_image_copy_from_mem(command_list_dev1, img_dev0, target_mem,
                                  event2);
  lzt::append_wait_on_events(command_list_dev1, 1, &event2);
  lzt::close_command_list(command_list_dev1);
  lzt::execute_command_lists(command_q_dev1, 1, &command_list_dev1, nullptr);
  lzt::synchronize(command_q_dev1, UINT64_MAX);

  // Copyback to host
  lzt::append_image_copy_to_mem(command_list_dev0, output_png.raw_data(),
                                img_dev0, nullptr);
  lzt::close_command_list(command_list_dev0);
  lzt::execute_command_lists(command_q_dev0, 1, &command_list_dev0, nullptr);
  lzt::synchronize(command_q_dev0, UINT64_MAX);

  // compare results
  EXPECT_EQ(input_png, output_png);

  lzt::destroy_event(event1);
  lzt::destroy_event(event2);
  lzt::free_memory(target_mem);
}

INSTANTIATE_TEST_CASE_P(P2PImageMemory, P2PImageCopyMemory,
                        testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                        ZE_MEMORY_TYPE_SHARED));

} // namespace
