/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_harness/test_harness_fence.hpp"
#include "logging/logging.hpp"
#include <thread>

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

using lzt::to_u32;

const int thread_iters = 200;
const size_t num_threads = 16;

void image_create_destroy_thread() {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  ze_image_handle_t image = nullptr;

  ze_image_desc_t image_descriptor = {};
  image_descriptor.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;

  image_descriptor.pNext = nullptr;
  image_descriptor.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  image_descriptor.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  image_descriptor.format.layout = ZE_IMAGE_FORMAT_LAYOUT_32;
  image_descriptor.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor.width = 128;
  image_descriptor.height = 128;
  image_descriptor.depth = 1;

  for (int i = 0; i < thread_iters; i++) {
    image = lzt::create_ze_image(image_descriptor);
    lzt::destroy_ze_image(image);
  }

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

void image_copy_thread(const ze_command_queue_handle_t &command_queue) {
  std::thread::id thread_id = std::this_thread::get_id();
  LOG_DEBUG << "child thread spawned with ID :: " << thread_id;

  auto command_list = lzt::create_command_list();

  ze_image_handle_t image1 = nullptr;
  ze_image_handle_t image2 = nullptr;

  ze_image_desc_t img_desc = {};
  img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  img_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  img_desc.type = ZE_IMAGE_TYPE_2D;
  img_desc.format = {ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
                     ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
                     ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
  img_desc.width = 128;
  img_desc.height = 128;
  img_desc.depth = 1;
  img_desc.arraylevels = 0;
  img_desc.miplevels = 0;

  image1 = lzt::create_ze_image(img_desc);
  image2 = lzt::create_ze_image(img_desc);
  uint32_t wdth = to_u32(img_desc.width),
           hght = to_u32(img_desc.height);
  lzt::ImagePNG32Bit src_img(wdth, hght), dst_img(wdth, hght);

  for (int i = 0; i < thread_iters; i++) {

    lzt::write_image_data_pattern(src_img, 1);
    lzt::append_image_copy_from_mem(command_list, image1,
                                    src_img.raw_data(), nullptr);

    lzt::append_barrier(command_list);
    lzt::append_image_copy(command_list, image2, image1, nullptr);
    lzt::append_barrier(command_list);
    lzt::append_image_copy_to_mem(command_list, dst_img.raw_data(), image2,
                                  nullptr);
    lzt::append_barrier(command_list);

    lzt::close_command_list(command_list);
    lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);
    lzt::synchronize(command_queue, UINT64_MAX);
    lzt::reset_command_list(command_list);

    ASSERT_EQ(0, memcmp(src_img.raw_data(), dst_img.raw_data(),
                        src_img.size_in_bytes()));
  }

  lzt::destroy_ze_image(image1);
  lzt::destroy_ze_image(image2);
  lzt::destroy_command_list(command_list);

  LOG_DEBUG << "child thread done with ID ::" << std::this_thread::get_id();
}

class zeImageCreateDestroyThreadTest : public ::testing::Test {};

LZT_TEST(
    zeImageCreateDestroyThreadTests,
    GivenMultipleThreadsWhenCreatingImagesThenImagesCreatedAndDestroyedSuccessfully) {

  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  std::vector<std::unique_ptr<std::thread>> threads;

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(image_create_destroy_thread)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }
}

class zeImageCopyThreadTest : public ::testing::Test {};

LZT_TEST(
    zeImageCopyThreadTests,
    GivenMultipleThreadsUsingSingleCommandQueueWhenCopyingImagesThenCopiedImagesAreCorrect) {
  if (!(lzt::image_support())) {
    LOG_INFO << "device does not support images, cannot run test";
    GTEST_SKIP();
  }
  LOG_DEBUG << "Total number of threads spawned ::" << num_threads;

  auto command_queue = lzt::create_command_queue();
  std::vector<std::unique_ptr<std::thread>> threads;

  for (size_t i = 0U; i < num_threads; i++) {
    threads.push_back(std::unique_ptr<std::thread>(
        new std::thread(image_copy_thread, command_queue)));
  }

  for (size_t i = 0U; i < num_threads; i++) {
    threads[i]->join();
  }

  lzt::destroy_command_queue(command_queue);
}

} // namespace
