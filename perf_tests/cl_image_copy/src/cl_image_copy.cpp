/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "cl_image_copy.h"

using namespace std;

void ClImageCopy::CreateImage() {
  // Create OpenCL image
  cl_image_format clImageFormat;
  clImageFormat.image_channel_order = clImageChannelOrder;
  clImageFormat.image_channel_data_type = clChannelDataType;

  cl_int errNum;

  // New in OpenCL 1.2, need to create image descriptor.
  cl_image_desc clImageDesc;
  clImageDesc.image_type = clImagetype;
  clImageDesc.image_width = width;
  clImageDesc.image_height = height;
  clImageDesc.image_depth = depth;
  clImageDesc.image_row_pitch = 0;
  clImageDesc.image_slice_pitch = 0;
  clImageDesc.num_mip_levels = 0;
  clImageDesc.num_samples = 0;
  clImageDesc.buffer = nullptr;

  // clCreateImage handles 1D, 2D, and 3D images but by default this test
  // creates 2D image
  clImage = clCreateImage(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                          &clImageFormat, &clImageDesc, src, &errNum);

  if (errNum != CL_SUCCESS) {
    throw std::runtime_error("CreateImage failed: " + std::to_string(errNum));
  }
}

void ClImageCopy::create_resources(void) {

  // Create context, queue
  cl_platform_id platform_id = nullptr;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret;
  cl_queue_properties queue_properties[] = {
      CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, 0};

  ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  if (ret != CL_SUCCESS) {
    throw std::runtime_error("clGetPlatformIDs failed: " + std::to_string(ret));
  }
  ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &clDevice,
                       &ret_num_devices);
  if (ret != CL_SUCCESS) {
    throw std::runtime_error("clGetDeviceIDs failed: " + std::to_string(ret));
  }

  // Create an OpenCL context
  clContext = clCreateContext(nullptr, 1, &clDevice, nullptr, nullptr, &ret);
  if (ret != CL_SUCCESS) {
    throw std::runtime_error("clCreateContext failed: " + std::to_string(ret));
  }

  clCommandQueueA = clCreateCommandQueueWithProperties(
      clContext, clDevice, queue_properties, nullptr);
  if (clCommandQueueA == nullptr) {
    throw std::runtime_error("clCreateCommandQueueWithProperties failed: ");
  }

  clCommandQueueB = clCreateCommandQueueWithProperties(
      clContext, clDevice, queue_properties, nullptr);
  if (clCommandQueueB == nullptr) {
    throw std::runtime_error("clCreateCommandQueueWithProperties failed: ");
  }

  imagesize = 4 * width * height * depth;

  // create host side src  and dst buffers
  src = new uint8_t[imagesize];
  dst = new uint8_t[imagesize];
  for (size_t i = 0; i < imagesize; ++i) {
    src[i] = static_cast<cl_int>(i);
    dst[i] = 0xff;
  }
  // By default it will create 2D Image unless updated from cmdline
  CreateImage();
}

void ClImageCopy::release_resources(void) {

  delete[] src;
  delete[] dst;

  if (clImage != 0)
    clReleaseMemObject(clImage);
  if (clCommandQueueA != 0)
    clReleaseCommandQueue(clCommandQueueA);
  if (clCommandQueueB != 0)
    clReleaseCommandQueue(clCommandQueueB);
  if (clContext != 0)
    clReleaseContext(clContext);
}

ClImageCopy ::ClImageCopy() {}

ClImageCopy ::~ClImageCopy() {}

bool ClImageCopy::is_json_output_enabled(void) {
  return JsonFileName.size() != 0;
}
void ClImageCopy::validate_data_buffer(void) {
  if (this->data_validation) {
    validRet = (0 == memcmp(src, dst, imagesize));
  }
}

void ClImageCopy::host2Device2HostEnqueue() {
  size_t region[3];
  size_t dst_origin[3];

  dst_origin[0] = xOffset;
  dst_origin[1] = yOffset;
  dst_origin[2] = zOffset;
  region[0] = width;
  region[1] = height;
  region[2] = depth;

  SUCCESS_OR_TERMINATE(clEnqueueWriteImage(clCommandQueueA, clImage, CL_FALSE,
                                           dst_origin, region, 0, 0, src, 0,
                                           nullptr, nullptr));
  SUCCESS_OR_TERMINATE(
      clEnqueueBarrierWithWaitList(clCommandQueueA, 0, nullptr, nullptr));
  SUCCESS_OR_TERMINATE(clEnqueueReadImage(clCommandQueueA, clImage, CL_FALSE,
                                          dst_origin, region, 0, 0, dst, 0,
                                          nullptr, nullptr));
}

// It is image copy measure from Host->Device->Host
void ClImageCopy::measureHost2Device2Host() {
  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec;
  long double total_time_s;
  long double total_data_transfer;

  create_resources();

  for (uint32_t i = 0; i < warm_up_iterations; i++) {
    this->host2Device2HostEnqueue();
    SUCCESS_OR_TERMINATE(clFinish(clCommandQueueA));
  }

  total_time_usec = 0;
  for (int i = 0; i < number_iterations; i++) {
    this->host2Device2HostEnqueue();

    timer.start();
    SUCCESS_OR_TERMINATE(clFinish(clCommandQueueA));
    timer.end();

    total_time_usec += timer.period_minus_overhead();
  }

  total_time_s = total_time_usec / 1e6;

  /* Buffer size is multiplied by 2 to account for the round trip image copy */
  total_data_transfer = (2 * imagesize * number_iterations) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;

  std::cout << gbps << " GBPS\n";

  validate_data_buffer();
  release_resources();
}

void ClImageCopy::parallelHost2DeviceEnqueueAll(size_t dst_origin[3],
                                                size_t region[3]) {
  for (int i = 0; i < num_image_copy; i++) {
    SUCCESS_OR_TERMINATE(clEnqueueWriteImage(clCommandQueueA, clImage, CL_FALSE,
                                             dst_origin, region, 0, 0, src, 0,
                                             nullptr, nullptr));
  }
}

void ClImageCopy::parallelHost2DeviceCopyImageToHost(size_t dst_origin[3],
                                                     size_t region[3]) {
  SUCCESS_OR_TERMINATE(clEnqueueReadImage(clCommandQueueB, clImage, CL_FALSE,
                                          dst_origin, region, 0, 0, dst, 0,
                                          nullptr, nullptr));
  SUCCESS_OR_TERMINATE(clFinish(clCommandQueueB));
}

void ClImageCopy::measureParallelHost2Device() {
  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;
  size_t dst_origin[3];
  size_t region[3];

  dst_origin[0] = xOffset;
  dst_origin[1] = yOffset;
  dst_origin[2] = zOffset;
  region[0] = width;
  region[1] = height;
  region[2] = depth;

  create_resources();

  // Warm up
  this->parallelHost2DeviceEnqueueAll(dst_origin, region);
  SUCCESS_OR_TERMINATE(clFinish(clCommandQueueA));

  // Measure the bandwidth of copy from host to device only
  total_time_usec = 0;
  for (int i = 0; i < number_iterations; i++) {
    this->parallelHost2DeviceEnqueueAll(dst_origin, region);

    timer.start();
    SUCCESS_OR_TERMINATE(clFinish(clCommandQueueA));
    timer.end();

    total_time_usec += timer.period_minus_overhead();
  }

  // Copy data from device to host to validate it
  this->parallelHost2DeviceCopyImageToHost(dst_origin, region);

  total_time_s = total_time_usec / 1e6;
  total_data_transfer = (imagesize * number_iterations * num_image_copy) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;
  std::cout << gbps << " GBPS\n";
  latency = total_time_usec /
            static_cast<long double>(number_iterations * num_image_copy);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Host->Device)" << std::endl;

  validate_data_buffer();
  release_resources();
}

void ClImageCopy::parallelDevice2HostCopyImageToDevice(size_t dst_origin[3],
                                                       size_t region[3]) {
  SUCCESS_OR_TERMINATE(clEnqueueWriteImage(clCommandQueueA, clImage, CL_FALSE,
                                           dst_origin, region, 0, 0, src, 0,
                                           nullptr, nullptr));
  SUCCESS_OR_TERMINATE(clFinish(clCommandQueueA));
}

void ClImageCopy::parallelDevice2HostEnqueue(size_t dst_origin[3],
                                             size_t region[3]) {
  SUCCESS_OR_TERMINATE(clEnqueueReadImage(clCommandQueueB, clImage, CL_FALSE,
                                          dst_origin, region, 0, 0, dst, 0,
                                          nullptr, nullptr));
}
void ClImageCopy::parallelDevice2HostEnqueueAll(size_t dst_origin[3],
                                                size_t region[3]) {
  for (int i = 0; i < num_image_copy; i++) {
    this->parallelDevice2HostEnqueue(dst_origin, region);
  }
}

void ClImageCopy::measureParallelDevice2Host() {

  size_t dst_origin[3];
  size_t region[3];
  Timer<std::chrono::microseconds::period> timer;
  long double total_time_usec = 0;
  long double total_time_s;
  long double total_data_transfer;

  create_resources();

  dst_origin[0] = xOffset;
  dst_origin[1] = yOffset;
  dst_origin[2] = zOffset;
  region[0] = width;
  region[1] = height;
  region[2] = depth;

  // Copying image to device first
  this->parallelDevice2HostCopyImageToDevice(dst_origin, region);

  // Warm up
  this->parallelDevice2HostEnqueue(dst_origin, region);
  SUCCESS_OR_TERMINATE(clFinish(clCommandQueueB));

  // Measure bandwidth for copy from device to host only
  total_time_usec = 0;
  for (int i = 0; i < number_iterations; i++) {
    this->parallelDevice2HostEnqueueAll(dst_origin, region);
    timer.start();
    SUCCESS_OR_TERMINATE(clFinish(clCommandQueueB));
    timer.end();

    total_time_usec += timer.period_minus_overhead();
  }

  total_time_s = total_time_usec / 1e6;
  total_data_transfer = (imagesize * number_iterations * num_image_copy) /
                        static_cast<long double>(1e9); /* Units in Gigabytes */

  gbps = total_data_transfer / total_time_s;

  std::cout << gbps << " GBPS\n";
  latency = total_time_usec /
            static_cast<long double>(number_iterations * num_image_copy);
  std::cout << std::setprecision(11) << latency << " us"
            << " (Latency: Device->Host)" << std::endl;

  validate_data_buffer();
  release_resources();
}

ClImageCopyLatency::ClImageCopyLatency() {
  width = 1;
  height = 1;
  depth = 1;
  clImageChannelOrder = CL_RGBA;
  clChannelDataType = CL_UNSIGNED_INT8;
}

void measure_bandwidth_Host2Device2Host(ClImageCopy &Imagecopy,
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

void measure_bandwidth_Host2Device(ClImageCopy &Imagecopy, ptree *test_ptree) {
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
}

void measure_bandwidth_Device2Host(ClImageCopy &Imagecopy, ptree *test_ptree) {
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
}

void measure_bandwidth(ClImageCopy &Imagecopy) {
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

void measure_latency_Host2Device(ClImageCopyLatency &imageCopyLatency,
                                 ptree *test_ptree) {
  if (imageCopyLatency.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << imageCopyLatency.width << "X" << imageCopyLatency.height
                     << "X" << imageCopyLatency.depth;
    test_ptree->put("Name",
                    "Host2Device: Measuring Latency for copying the image ");
    test_ptree->put("Image size", Image_dimensions.str());
    test_ptree->put("Image type", level_zero_tests::to_string(
                                      (cl_uint)imageCopyLatency.clImagetype));
    test_ptree->put("Image Channel order",
                    level_zero_tests::to_string(
                        (cl_uint)imageCopyLatency.clImageChannelOrder));
  } else {
    std::cout
        << "Host2Device: Measuring Bandwidth/Latency for copying the image "
           "buffer size "
        << imageCopyLatency.width << "X" << imageCopyLatency.height << "X"
        << imageCopyLatency.depth << "   Image type=  "
        << level_zero_tests::to_string((cl_uint)imageCopyLatency.clImagetype)
        << "  Image Channel order=  "
        << level_zero_tests::to_string(
               (cl_uint)imageCopyLatency.clImageChannelOrder)
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

void measure_latency_Device2Host(ClImageCopyLatency &imageCopyLatency,
                                 ptree *test_ptree) {
  if (imageCopyLatency.is_json_output_enabled()) {
    std::stringstream Image_dimensions;
    Image_dimensions << imageCopyLatency.width << "X" << imageCopyLatency.height
                     << "X" << imageCopyLatency.depth;
    test_ptree->put("Name",
                    "Device2Host: Measuring Latency for copying the image");
    test_ptree->put("Image size", Image_dimensions.str());
    test_ptree->put("Image type", level_zero_tests::to_string(
                                      (cl_uint)imageCopyLatency.clImagetype));
    test_ptree->put("Image Channel order",
                    level_zero_tests::to_string(
                        (cl_uint)imageCopyLatency.clImageChannelOrder));
  } else {
    std::cout
        << "Device2Host: Measuring Bandwidth/Latency for copying the image "
           "buffer size "
        << imageCopyLatency.width << "X" << imageCopyLatency.height << "X"
        << imageCopyLatency.depth << "   Image type=  "
        << level_zero_tests::to_string((cl_uint)imageCopyLatency.clImagetype)
        << "  Image Channel order=  "
        << level_zero_tests::to_string(
               (cl_uint)imageCopyLatency.clImageChannelOrder)
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
void measure_latency(ClImageCopyLatency &imageCopyLatency) {
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
  ClImageCopy Imagecopy;
  SUCCESS_OR_TERMINATE(Imagecopy.parse_command_line(argc, argv));
  measure_bandwidth(Imagecopy);
  ClImageCopyLatency imageCopyLatency;
  imageCopyLatency.JsonFileName =
      Imagecopy.JsonFileName; // need to add latency values to the same file
  measure_latency(imageCopyLatency);
  return 0;
}
