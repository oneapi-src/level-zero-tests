/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef CL_IMAGE_COPY_H
#define CL_IMAGE_COPY_H

#include <memory>
#include <string>
#include <utility>
#include <CL/cl_ext.h>

#include "common.hpp"
#include <assert.h>
#include <iomanip>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using namespace std;
using namespace pt;

class ClImageCopy {

public:
  uint32_t width = 2048;
  uint32_t height = 2048;
  uint32_t depth = 1;
  uint32_t xOffset = 0;
  uint32_t yOffset = 0;
  uint32_t zOffset = 0;
  uint32_t number_iterations = 50;
  uint32_t warm_up_iterations = 10;
  uint32_t num_image_copy = 100;
  uint32_t data_validation = 0;
  bool validRet = false;
  long double gbps;
  long double latency;
  ptree param_array;
  ptree main_tree;
  string JsonFileName;

  cl_channel_order clImageChannelOrder = CL_RGBA;
  cl_channel_type clChannelDataType = CL_UNSIGNED_INT8;
  cl_mem_object_type clImagetype = CL_MEM_OBJECT_IMAGE2D;

  ClImageCopy();
  ~ClImageCopy();
  void measureHost2Device2Host();
  void measureParallelHost2Device();
  void measureParallelDevice2Host();
  int parse_command_line(int argc, char **argv);
  bool is_json_output_enabled();

private:
  cl_context clContext;
  cl_device_id clDevice;
  cl_command_queue clCommandQueueA;
  cl_command_queue clCommandQueueB;
  cl_mem clImage;
  size_t imagesize;
  uint8_t *src, *dst;

  void CreateImage();
  void create_resources();
  void release_resources();
  void validate_data_buffer();

  void host2Device2HostEnqueue();

  void parallelHost2DeviceEnqueueAll(size_t dst_origin[3], size_t region[3]);
  void parallelHost2DeviceCopyImageToHost(size_t dst_origin[3],
                                          size_t region[3]);

  void parallelDevice2HostEnqueue(size_t dst_origin[3], size_t region[3]);
  void parallelDevice2HostEnqueueAll(size_t dst_origin[3], size_t region[3]);
  void parallelDevice2HostCopyImageToDevice(size_t dst_origin[3],
                                            size_t region[3]);

  cl_channel_type to_channel_datatype(std::string channel_datatype);
  cl_channel_order to_channel_order(std::string channel_order);
  cl_mem_object_type to_image_type(std::string image_type);
};

class ClImageCopyLatency : public ClImageCopy {
public:
  ClImageCopyLatency();
};
namespace level_zero_tests {

// for channel order, channel type and mem_object type
std::string to_string(const cl_uint cl_var);
std::string to_string(const cl_mem_flags flags);
} // namespace level_zero_tests

#endif /* CL_IMAGE_COPY_H */
