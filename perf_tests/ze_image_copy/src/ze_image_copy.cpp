/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_image_copy.h"
#include <cassert>

ZeImageCopy ::ZeImageCopy() {

  benchmark = new ZeApp();
  benchmark->singleDeviceInit();

  /*
   * For single device, create command Queue, command list_a, command_list_b and
   * image with ze_app
   */

  benchmark->commandQueueCreate(0, &this->command_queue);
  benchmark->commandListCreate(&this->command_list);
  benchmark->commandListCreate(&this->command_list_a);
  benchmark->commandListCreate(&this->command_list_b);
}

ZeImageCopy ::~ZeImageCopy() {
  benchmark->commandQueueDestroy(this->command_queue);
  benchmark->commandListDestroy(this->command_list);
  benchmark->commandListDestroy(this->command_list_a);
  benchmark->commandListDestroy(this->command_list_b);
  benchmark->singleDeviceCleanup();

  delete benchmark;
}

bool ZeImageCopy::is_json_output_enabled(void) {
  return JsonFileName.size() != 0;
}

uint64_t roundToMultipleOf(uint64_t number, uint64_t base, uint64_t maxValue) {
  uint64_t n = (number > maxValue) ? maxValue : number;
  return (n / base) * base;
}

void ZeImageCopy::test_initialize(void) {
  size_t buffer_size_tmp = level_zero_tests::num_bytes_per_pixel(Imagelayout) *
                           width * height * depth;
  buffer_size = roundToMultipleOf(buffer_size_tmp, 8, SIZE_MAX);
  if (buffer_size == 0 && buffer_size_tmp > 0) {
    buffer_size = buffer_size_tmp;
  }
  region = {xOffset, yOffset, zOffset, width, height, depth};
  formatDesc = {Imagelayout,
                Imageformat,
                ZE_IMAGE_FORMAT_SWIZZLE_R,
                ZE_IMAGE_FORMAT_SWIZZLE_G,
                ZE_IMAGE_FORMAT_SWIZZLE_B,
                ZE_IMAGE_FORMAT_SWIZZLE_A};

  imageDesc = {};
  imageDesc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  imageDesc.flags = Imageflags;
  imageDesc.type = Imagetype;
  imageDesc.format = formatDesc;
  imageDesc.width = width;
  imageDesc.height = height;
  imageDesc.depth = depth;
  imageDesc.arraylevels = 0;
  imageDesc.miplevels = 0;

#ifdef _WIN32
  srcBuffer = static_cast<uint8_t *>(_aligned_malloc(buffer_size, 64));
  dstBuffer = static_cast<uint8_t *>(_aligned_malloc(buffer_size, 64));
#else
  srcBuffer = static_cast<uint8_t *>(aligned_alloc(64, buffer_size));
  dstBuffer = static_cast<uint8_t *>(aligned_alloc(64, buffer_size));
#endif

  if (!srcBuffer || !dstBuffer) {
    std::cerr << "ERROR : " << __FILE__ << ":" << __LINE__
              << " Failed to allocate aligned memory" << std::endl;
    std::terminate();
  }

  for (size_t i = 0; i < this->buffer_size; ++i) {
    srcBuffer[i] = static_cast<uint8_t>(i);
    dstBuffer[i] = 0xff;
  }

  benchmark->imageCreate(&imageDesc, &this->image);

  // Besides having events for image copies, one more event is reserved to
  // indicate completion event of entire batch of commands.
  num_wait_events = num_image_copies + 1;
  hdevice_event = new ze_event_handle_t[num_wait_events];
  event_pool = benchmark->create_event_pool(num_wait_events,
                                            ZE_EVENT_POOL_FLAG_HOST_VISIBLE);
  for (int i = 0; i < num_wait_events; i++) {
    benchmark->create_event(event_pool, hdevice_event[i], i);
    zeEventHostReset(hdevice_event[i]);
  }
}

void ZeImageCopy::test_cleanup(void) {
#ifdef _WIN32
  _aligned_free(srcBuffer);
  _aligned_free(dstBuffer);
#else
  free(srcBuffer);
  free(dstBuffer);
#endif
  benchmark->imageDestroy(this->image);
  image = nullptr;
  for (int i = 0; i < num_wait_events; i++)
    benchmark->destroy_event(hdevice_event[i]);
  benchmark->destroy_event_pool(this->event_pool);
  event_pool = nullptr;
  delete[] hdevice_event;
}

void ZeImageCopy::reset_all_events(void) {
  for (int i = 0; i < num_wait_events; i++) {
    zeEventHostReset(hdevice_event[i]);
  }
}

void ZeImageCopy::validate_data_buffer(void) {
  if (this->data_validation) {
    validRet =
        (0 == memcmp(this->srcBuffer, this->dstBuffer, this->buffer_size));
  }
}

// It is image copy measure from Host->Device->Host
void ZeImageCopy::measureHost2Device2Host() {

  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;

  ze_result_t result = ZE_RESULT_SUCCESS;
  this->test_initialize();

  // Copy from srcBuffer->Image->dstBuffer, so at the end dstBuffer = srcBuffer
  benchmark->commandListAppendImageCopyFromMemory(command_list, image,
                                                  srcBuffer, &this->region);
  benchmark->commandListAppendBarrier(command_list);
  benchmark->commandListAppendImageCopyToMemory(command_list, dstBuffer, image,
                                                &this->region);
  benchmark->commandListClose(command_list);

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list);
    benchmark->commandQueueSynchronize(command_queue);
  }

  // Measure the bandwidth of copy from host to device to host only
  for (int i = 0; i < num_iterations; i++) {

    timer.start();

    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(command_queue, UINT64_MAX));

    timer.end();
    total_time_usec += timer.period_minus_overhead();
  }

  total_time_s = total_time_usec / 1e6;

  /* Buffer size is multiplied by 2 to account for the round trip image copy */
  total_data_transfer = (2 * this->buffer_size * num_iterations) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;

  std::cout << gbps << " GBPS\n";
  this->validate_data_buffer();
  this->test_cleanup();
}

void ZeImageCopy::measureParallelHost2Device() {

  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;

  ze_result_t result = ZE_RESULT_SUCCESS;
  this->test_initialize();

  // Copy from srcBuffer->Image->dstBuffer, so at the end dstBuffer = srcBuffer
  benchmark->commandListReset(command_list_a);
  for (int i = 0; i < num_image_copies; i++) {
    benchmark->commandListAppendImageCopyFromMemory(command_list_a, image,
                                                    srcBuffer, &this->region);
  }
  benchmark->commandListClose(command_list_a);

  benchmark->commandListReset(command_list_b);
  benchmark->commandListAppendImageCopyToMemory(command_list_b, dstBuffer,
                                                image, &this->region);
  benchmark->commandListClose(command_list_b);

  /* Warm up */

  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_a);
  benchmark->commandQueueSynchronize(command_queue);

  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_b);
  benchmark->commandQueueSynchronize(command_queue);

  for (int i = 0; i < num_iterations; i++) {

    // Measure the bandwidth of copy from host to device only
    timer.start();
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list_a, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
    timer.end();

    total_time_usec += timer.period_minus_overhead();
  }

  // The below commands for command_list_b for final validation at the end
  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_b);
  benchmark->commandQueueSynchronize(command_queue);

  total_time_s = total_time_usec / 1e6;

  total_data_transfer = (buffer_size * num_image_copies * num_iterations) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;
  std::cout << gbps << " GBPS\n";
  latency = total_time_usec /
            static_cast<long double>(num_image_copies * num_iterations);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Host->Device)" << std::endl;
  this->validate_data_buffer();
  this->test_cleanup();
}

void ZeImageCopy::measureParallelDevice2Host() {

  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;

  ze_result_t result = ZE_RESULT_SUCCESS;
  this->test_initialize();

  // Copy from srcBuffer->Image->dstBuffer, so at the end dstBuffer = srcBuffer

  // commandListReset to make sure resetting the command_list_a from previous
  // operations on host2device
  benchmark->commandListReset(command_list_a);
  benchmark->commandListAppendImageCopyFromMemory(command_list_a, image,
                                                  srcBuffer, &this->region);
  benchmark->commandListClose(command_list_a);

  // commandListReset to make sure resetting the command_list_b from previous
  // operations on host2device
  benchmark->commandListReset(command_list_b);
  for (int i = 0; i < num_image_copies; i++) {
    benchmark->commandListAppendImageCopyToMemory(command_list_b, dstBuffer,
                                                  image, &this->region);
  }
  benchmark->commandListClose(command_list_b);

  /* Warm up */
  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_a);
  benchmark->commandQueueSynchronize(command_queue);

  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_b);
  benchmark->commandQueueSynchronize(command_queue);

  for (int i = 0; i < num_iterations; i++) {

    // measure the bandwidth of copy from device to host only
    timer.start();
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list_b, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(command_queue, UINT64_MAX));
    timer.end();

    total_time_usec += timer.period_minus_overhead();
  }
  total_time_s = total_time_usec / 1e6;

  total_data_transfer =
      (this->buffer_size * num_image_copies * num_iterations) /
      static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;
  std::cout << gbps << " GBPS\n";
  latency = total_time_usec /
            static_cast<long double>(num_image_copies * num_iterations);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Device->Host)" << std::endl;
  this->validate_data_buffer();
  this->test_cleanup();
}

void ZeImageCopy::measureSerialHost2Device() {

  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;

  ze_result_t result = ZE_RESULT_SUCCESS;

  this->test_initialize();
  benchmark->commandListReset(command_list_a);
  benchmark->commandListReset(command_list_b);

  // Queue all image copies where it depends on the previously queued event
  // to start executing. When the first event is signaled, the entire batch
  // of commands starts running.
  assert(("num_wait_events is greater than num_image_copies by 1 as the"
          " last event is used to indicate all commands have completed.",
          num_image_copies + 1 == num_wait_events));
  ze_event_handle_t *first_event = &hdevice_event[0];
  ze_event_handle_t *last_event = &hdevice_event[num_wait_events - 1];
  benchmark->commandListAppendWaitOnEvents(command_list_a, 1, first_event);
  for (int i = 0; i < num_image_copies; i++) {
    benchmark->commandListAppendImageCopyFromMemory(
        command_list_a, image, srcBuffer, &this->region,
        hdevice_event[i + 1] // Signal this event upon image copy completion
    );

    // Last event in the command list indicates command list completion
    benchmark->commandListAppendWaitOnEvents(command_list_a, 1,
                                             &hdevice_event[i + 1]);
  }
  benchmark->commandListClose(command_list_a);

  // Queue commands on a different command list to copy data from device to host
  // to validate it.
  benchmark->commandListAppendImageCopyToMemory(command_list_b, dstBuffer,
                                                image, &this->region);
  benchmark->commandListClose(command_list_b);

  // Warm up
  for (int j = 0; j < warm_up_iterations; j++) {
    // Launch all command queued. They will wait to be executed until after
    // an event is signaled.
    benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                              &command_list_a);
    benchmark->hostEventSignal(*first_event);
    benchmark->hostSynchronize(*last_event);
    benchmark->commandQueueSynchronize(command_queue);

    // Since all event have been signaled, we have to reset them to reuse them
    reset_all_events();
  }

  // Measure the bandwidth of copy from host to device only
  total_time_usec = 0;
  for (int j = 0; j < num_iterations; j++) {
    benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                              &command_list_a);
    timer.start();
    SUCCESS_OR_TERMINATE(zeEventHostSignal(*first_event));
    SUCCESS_OR_TERMINATE(zeEventHostSynchronize(*last_event, ~0));
    timer.end();
    benchmark->commandQueueSynchronize(command_queue);

    // Since all event have been signaled, we have to reset them to reuse them
    reset_all_events();
    total_time_usec += timer.period_minus_overhead();
  }

  // Copy data from device to host to validate it
  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_b);
  benchmark->commandQueueSynchronize(command_queue);

  total_time_s = total_time_usec / 1e6;

  total_data_transfer = (buffer_size * num_iterations) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;
  std::cout << gbps << " GBPS\n";
  latency = total_time_usec / static_cast<long double>(num_iterations);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Host->Device)" << std::endl;

  this->validate_data_buffer();
  this->test_cleanup();
}

void ZeImageCopy::measureSerialDevice2Host() {

  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec;
  long double total_time_s;
  long double total_data_transfer;

  ze_result_t result = ZE_RESULT_SUCCESS;
  this->test_initialize();
  benchmark->commandListReset(command_list_a);
  benchmark->commandListReset(command_list_b);

  // Copy data from host to device, so that it can be verified
  benchmark->commandListAppendImageCopyFromMemory(command_list_a, image,
                                                  srcBuffer, &this->region);
  benchmark->commandListClose(command_list_a);
  benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list_a);
  benchmark->commandQueueSynchronize(command_queue);

  // Queue all image copies where it depends on the previously queued event
  // to start executing. When the first event is signaled, the entire batch
  // of commands starts running.
  assert(("num_wait_events is greater than num_image_copies by 1 as the"
          " last event is used to indicate all commands have completed.",
          num_image_copies + 1 == num_wait_events));
  ze_event_handle_t *first_event = &hdevice_event[0];
  ze_event_handle_t *last_event = &hdevice_event[num_wait_events - 1];
  benchmark->commandListAppendWaitOnEvents(command_list_b, 1, first_event);
  for (int i = 0; i < num_image_copies; i++) {
    benchmark->commandListAppendImageCopyToMemory(
        command_list_b, dstBuffer, image, &this->region,
        hdevice_event[i + 1] // Signal this event upon image copy completion
    );

    // Last event in the command list indicates command list completion
    benchmark->commandListAppendWaitOnEvents(command_list_b, 1,
                                             &hdevice_event[i + 1]);
  }
  benchmark->commandListClose(command_list_b);

  // Warm up
  for (int i = 0; i < warm_up_iterations; i++) {
    // Launch all command queued. They will wait to be executed until after
    // an event is signaled.
    benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                              &command_list_b);
    benchmark->hostEventSignal(*first_event);
    benchmark->hostSynchronize(*last_event);
    benchmark->commandQueueSynchronize(command_queue);

    // Since all event have been signaled, we have to reset them to reuse them
    reset_all_events();
  }

  // Measure the bandwidth of copy from device to host only
  total_time_usec = 0;
  for (int i = 0; i < num_iterations; i++) {
    // Measure the bandwidth of copy from device to host only
    benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                              &command_list_b);

    timer.start();
    SUCCESS_OR_TERMINATE(zeEventHostSignal(*first_event));
    SUCCESS_OR_TERMINATE(zeEventHostSynchronize(*last_event, ~0));
    timer.end();
    benchmark->commandQueueSynchronize(command_queue);

    // Since all event have been signaled, we have to reset them to reuse them
    reset_all_events();
    total_time_usec += timer.period_minus_overhead();
  }

  total_time_s = total_time_usec / 1e6;

  total_data_transfer = (this->buffer_size * num_iterations) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;
  std::cout << gbps << " GBPS\n";
  latency = total_time_usec / static_cast<long double>(num_iterations);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Device->Host)" << std::endl;
  this->validate_data_buffer();
  this->test_cleanup();
}

ZeImageCopyLatency::ZeImageCopyLatency() {
  width = 1;
  height = 1;
  depth = 1;
  Imagelayout = ZE_IMAGE_FORMAT_LAYOUT_8;
  Imageformat = ZE_IMAGE_FORMAT_TYPE_UINT;
}

void measure_bandwidth_Host2Device2Host(ZeImageCopy &Imagecopy,
                                        ptree *test_ptree) {

  if (Imagecopy.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << Imagecopy.width << "X" << Imagecopy.height << "X"
                     << Imagecopy.depth;
    test_ptree->put(
        "Name", "Host2Device2Host: Bandwidth copying from Host->Device->Host ");
    test_ptree->put("Image size", Image_dimensions.str());
  } else {
    std::cout << "Host2Device2Host: Measuring Bandwidth for copying the "
                 "image buffer size "
              << Imagecopy.width << "X" << Imagecopy.height << "X"
              << Imagecopy.depth << " from Host->Device->Host" << std::endl;
  }

  Imagecopy.measureHost2Device2Host();

  if (Imagecopy.is_json_output_enabled()) {
    test_ptree->put("GBPS", Imagecopy.gbps);
    if (Imagecopy.data_validation)
      test_ptree->put("Result", (Imagecopy.validRet ? "PASSED" : "FAILED"));
  } else {
    if (Imagecopy.data_validation) {
      std::cout << "  Results: Data validation "
                << (Imagecopy.validRet ? "PASSED" : "FAILED") << std::endl;
      std::cout << std::endl;
    }
  }
}

void measure_bandwidth_Host2Device(ZeImageCopy &Imagecopy, ptree *test_ptree) {
  if (Imagecopy.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << Imagecopy.width << "X" << Imagecopy.height << "X"
                     << Imagecopy.depth;
    test_ptree->put("Name", "Host2Device: Bandwidth copying from Host->Device");
    test_ptree->put("Image size", Image_dimensions.str());
  } else {
    std::cout
        << "Host2Device: Measuring Bandwidth/Latency for copying the image "
           "buffer size "
        << Imagecopy.width << "X" << Imagecopy.height << "X" << Imagecopy.depth
        << " from Host->Device" << std::endl;
  }

  Imagecopy.measureParallelHost2Device();

  if (Imagecopy.is_json_output_enabled()) {
    test_ptree->put("GBPS", Imagecopy.gbps);
    if (Imagecopy.data_validation) {
      test_ptree->put("Result", (Imagecopy.validRet ? "PASSED" : "FAILED"));
    }
  } else {
    if (Imagecopy.data_validation) {
      std::cout << "  Results: Data validation "
                << (Imagecopy.validRet ? "PASSED" : "FAILED") << std::endl;
      std::cout << std::endl;
    }
  }
  Imagecopy.measureSerialHost2Device();
}

void measure_bandwidth_Device2Host(ZeImageCopy &Imagecopy, ptree *test_ptree) {
  if (Imagecopy.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << Imagecopy.width << "X" << Imagecopy.height << "X"
                     << Imagecopy.depth;
    test_ptree->put("Name", "Device2Host: Bandwidth copying from Device->Host");
    test_ptree->put("Image size", Image_dimensions.str());
  } else {
    std::cout
        << "Device2Host: Measurign Bandwidth/Latency for copying the image "
           "buffer size "
        << Imagecopy.width << "X" << Imagecopy.height << "X" << Imagecopy.depth
        << " from Device->Host" << std::endl;
  }

  Imagecopy.measureParallelDevice2Host();

  if (Imagecopy.is_json_output_enabled()) {
    test_ptree->put("GBPS", Imagecopy.gbps);
    if (Imagecopy.data_validation)
      test_ptree->put("Result", (Imagecopy.validRet ? "PASSED" : "FAILED"));
  } else {
    if (Imagecopy.data_validation) {
      std::cout << "  Results: Data validation "
                << (Imagecopy.validRet ? "PASSED" : "FAILED") << std::endl;
      std::cout << std::endl;
    }
  }

  Imagecopy.measureSerialDevice2Host();
}

void measure_bandwidth(ZeImageCopy &Imagecopy) {
  ptree ptree_Host2Device2Host;
  ptree ptree_Host2Device;
  ptree ptree_Device2Host;
  ptree ptree_main;

  measure_bandwidth_Host2Device2Host(Imagecopy, &ptree_Host2Device2Host);
  measure_bandwidth_Host2Device(Imagecopy, &ptree_Host2Device);
  measure_bandwidth_Device2Host(Imagecopy, &ptree_Device2Host);

  if (Imagecopy.is_json_output_enabled()) {
    Imagecopy.param_array.push_back(std::make_pair("", ptree_Host2Device2Host));
    Imagecopy.param_array.push_back(std::make_pair("", ptree_Host2Device));
    Imagecopy.param_array.push_back(std::make_pair("", ptree_Device2Host));
    ptree_main.put_child("Performance Benchmark.bandwidth",
                         Imagecopy.param_array);
    pt::write_json(Imagecopy.JsonFileName.c_str(), ptree_main);
  }
}

void measure_latency_Host2Device(ZeImageCopyLatency &imageCopyLatency,
                                 ptree *test_ptree) {
  if (imageCopyLatency.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << imageCopyLatency.width << "X" << imageCopyLatency.height
                     << "X" << imageCopyLatency.depth;
    test_ptree->put("Name",
                    "Host2Device: Measuring Latency for copying the image ");
    test_ptree->put("Image size", Image_dimensions.str());
    test_ptree->put("Image format", imageCopyLatency.Imageformat);
    test_ptree->put("Image Layout", imageCopyLatency.Imagelayout);
  } else {
    std::cout
        << "Host2Device: Measuring Bandwidth/Latency for copying the image "
           "buffer size "
        << imageCopyLatency.width << "X" << imageCopyLatency.height << "X"
        << imageCopyLatency.depth << "   Image format=  "
        << level_zero_tests::to_string(imageCopyLatency.Imageformat)
        << "  Image Layout=  "
        << level_zero_tests::to_string(imageCopyLatency.Imagelayout)
        << "  from Host->Device" << std::endl;
  }

  imageCopyLatency.measureParallelHost2Device();

  if (imageCopyLatency.is_json_output_enabled()) {
    test_ptree->put("Latency", imageCopyLatency.latency);
    if (imageCopyLatency.data_validation)
      test_ptree->put("Result",
                      (imageCopyLatency.validRet ? "PASSED" : "FAILED"));
  }
}

void measure_latency_Device2Host(ZeImageCopyLatency &imageCopyLatency,
                                 ptree *test_ptree) {
  if (imageCopyLatency.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << imageCopyLatency.width << "X" << imageCopyLatency.height
                     << "X" << imageCopyLatency.depth;
    test_ptree->put("Name",
                    "Device2Host: Measuring Latency for copying the image");
    test_ptree->put("Image size", Image_dimensions.str());
    test_ptree->put("Image format", imageCopyLatency.Imageformat);
    test_ptree->put("Image Layout", imageCopyLatency.Imagelayout);
  } else {
    std::cout
        << "Device2Host: Measuring Bandwidth/Latency for copying the image "
           "buffer size "
        << imageCopyLatency.width << "X" << imageCopyLatency.height << "X"
        << imageCopyLatency.depth << "   Image format=  "
        << level_zero_tests::to_string(imageCopyLatency.Imageformat)
        << "  Image Layout=  "
        << level_zero_tests::to_string(imageCopyLatency.Imagelayout)
        << "  from Device->Host" << std::endl;
  }

  imageCopyLatency.measureParallelDevice2Host();

  if (imageCopyLatency.is_json_output_enabled()) {
    test_ptree->put("Latency", imageCopyLatency.latency);
    if (imageCopyLatency.data_validation)
      test_ptree->put("Result",
                      (imageCopyLatency.validRet ? "PASSED" : "FAILED"));
  }
}

// Measuring latency for 1x1x1 image from Dev->Host & Host->Dev
void measure_latency(ZeImageCopyLatency &imageCopyLatency) {
  ptree ptree_Host2Device;
  ptree ptree_Device2Host;
  ptree ptree_main;
  ptree ptree_param_array;

  if (imageCopyLatency.is_json_output_enabled()) {
    pt::read_json(imageCopyLatency.JsonFileName, ptree_main);
  }
  measure_latency_Host2Device(imageCopyLatency, &ptree_Host2Device);
  measure_latency_Device2Host(imageCopyLatency, &ptree_Device2Host);

  if (imageCopyLatency.is_json_output_enabled()) {
    ptree_param_array.push_back(std::make_pair("", ptree_Host2Device));
    ptree_param_array.push_back(std::make_pair("", ptree_Device2Host));
    ptree_main.put_child("Performance Benchmark.latency", ptree_param_array);
    pt::write_json(imageCopyLatency.JsonFileName.c_str(), ptree_main);
  }
}

int main(int argc, char **argv) {
  ZeImageCopy Imagecopy;
  SUCCESS_OR_TERMINATE(Imagecopy.parse_command_line(argc, argv));
  measure_bandwidth(Imagecopy);

  ZeImageCopyLatency imageCopyLatency;
  imageCopyLatency.JsonFileName =
      Imagecopy.JsonFileName; // need to add latency values to the same file
  measure_latency(imageCopyLatency);

  std::cout << std::flush;

  return 0;
}
