/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef ZE_IMAGE_COPY_H
#define ZE_IMAGE_COPY_H

#include "common.hpp"
#include <level_zero/ze_api.h>
#include "ze_app.hpp"

#include <assert.h>
#include <iomanip>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>
#include "utils/utils.hpp"

namespace po = boost::program_options;
namespace pt = boost::property_tree;
using namespace pt;

class ZeImageCopy {
public:
  uint32_t width = 2048;
  uint32_t height = 2048;
  uint32_t depth = 1;
  uint32_t xOffset = 0;
  uint32_t yOffset = 0;
  uint32_t zOffset = 0;
  uint32_t num_iterations = 50;
  uint32_t warm_up_iterations = 10;
  uint32_t num_image_copies = 100;
  uint32_t data_validation = 0;
  bool validRet = false;
  long double gbps;
  long double latency;
  ptree param_array;
  ze_image_format_layout_t Imagelayout = ZE_IMAGE_FORMAT_LAYOUT_8;
  ze_image_flag_t Imageflags = ZE_IMAGE_FLAG_PROGRAM_READ;
  ze_image_type_t Imagetype = ZE_IMAGE_TYPE_2D;
  ze_image_format_type_t Imageformat = ZE_IMAGE_FORMAT_TYPE_UINT;
  std::string JsonFileName;
  ZeImageCopy();
  ~ZeImageCopy();
  void measureHost2Device2Host();
  void measureParallelHost2Device();
  void measureParallelDevice2Host();
  void measureSerialHost2Device();
  void measureSerialDevice2Host();
  int parse_command_line(int argc, char **argv);
  bool is_json_output_enabled();

private:
  void initialize_buffer(void);
  void test_initialize(void);
  void test_cleanup(void);
  void validate_data_buffer(void);
  void reset_all_events(void);

  ZeApp *benchmark;
  ze_command_queue_handle_t command_queue;
  ze_command_list_handle_t command_list;
  ze_command_list_handle_t command_list_a;
  ze_command_list_handle_t command_list_b;
  ze_image_handle_t image;
  ze_event_handle_t *hdevice_event;
  ze_event_pool_handle_t event_pool;

  ze_image_region_t region;
  ze_image_format_desc_t formatDesc;
  ze_image_desc_t imageDesc;
  uint8_t *srcBuffer;
  uint8_t *dstBuffer;
  size_t buffer_size;
  uint32_t num_wait_events;
};

class ZeImageCopyLatency : public ZeImageCopy {
public:
  ZeImageCopyLatency();
};

#endif /* ZE_IMAGE_COPY_H */
