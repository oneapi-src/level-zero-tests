/*
 *
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "utils/utils_string.hpp"
#include "logging/logging.hpp"

namespace {
std::stringstream &to_string_helper(std::stringstream &ss) {
  if (ss.str().empty()) {
    ss << "|NONE|";
  }
  return ss;
}
} // namespace
namespace level_zero_tests {

std::string to_string(const ze_api_version_t version) {
  std::stringstream ss;
  ss << ZE_MAJOR_VERSION(version) << "." << ZE_MINOR_VERSION(version);
  return ss.str();
}

std::string to_string(const ze_result_t result) {
  switch (result) {
  case ZE_RESULT_SUCCESS:
    return "ZE_RESULT_SUCCESS";
  case ZE_RESULT_NOT_READY:
    return "ZE_RESULT_NOT_READY";
  case ZE_RESULT_ERROR_DEVICE_LOST:
    return "ZE_RESULT_ERROR_DEVICE_LOST";
  case ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY:
    return "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY";
  case ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY:
    return "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY";
  case ZE_RESULT_ERROR_MODULE_BUILD_FAILURE:
    return "ZE_RESULT_ERROR_MODULE_BUILD_FAILURE";
  case ZE_RESULT_ERROR_MODULE_LINK_FAILURE:
    return "ZE_RESULT_ERROR_MODULE_LINK_FAILURE";
  case ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET:
    return "ZE_RESULT_ERROR_DEVICE_REQUIRES_RESET";
  case ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE:
    return "ZE_RESULT_ERROR_DEVICE_IN_LOW_POWER_STATE";
  case ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS:
    return "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS";
  case ZE_RESULT_ERROR_NOT_AVAILABLE:
    return "ZE_RESULT_ERROR_NOT_AVAILABLE";
  case ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE:
    return "ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE";
  case ZE_RESULT_WARNING_DROPPED_DATA:
    return "ZE_RESULT_WARNING_DROPPED_DATA";
  case ZE_RESULT_ERROR_UNINITIALIZED:
    return "ZE_RESULT_ERROR_UNINITIALIZED";
  case ZE_RESULT_ERROR_UNSUPPORTED_VERSION:
    return "ZE_RESULT_ERROR_UNSUPPORTED_VERSION";
  case ZE_RESULT_ERROR_UNSUPPORTED_FEATURE:
    return "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
  case ZE_RESULT_ERROR_INVALID_ARGUMENT:
    return "ZE_RESULT_ERROR_INVALID_ARGUMENT";
  case ZE_RESULT_ERROR_INVALID_NULL_HANDLE:
    return "ZE_RESULT_ERROR_INVALID_NULL_HANDLE";
  case ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE:
    return "ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE";
  case ZE_RESULT_ERROR_INVALID_NULL_POINTER:
    return "ZE_RESULT_ERROR_INVALID_NULL_POINTER";
  case ZE_RESULT_ERROR_INVALID_SIZE:
    return "ZE_RESULT_ERROR_INVALID_SIZE";
  case ZE_RESULT_ERROR_UNSUPPORTED_SIZE:
    return "ZE_RESULT_ERROR_UNSUPPORTED_SIZE";
  case ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT:
    return "ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT";
  case ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT:
    return "ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT";
  case ZE_RESULT_ERROR_INVALID_ENUMERATION:
    return "ZE_RESULT_ERROR_INVALID_ENUMERATION";
  case ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION:
    return "ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION";
  case ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT:
    return "ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT";
  case ZE_RESULT_ERROR_INVALID_NATIVE_BINARY:
    return "ZE_RESULT_ERROR_INVALID_NATIVE_BINARY";
  case ZE_RESULT_ERROR_INVALID_GLOBAL_NAME:
    return "ZE_RESULT_ERROR_INVALID_GLOBAL_NAME";
  case ZE_RESULT_ERROR_INVALID_KERNEL_NAME:
    return "ZE_RESULT_ERROR_INVALID_KERNEL_NAME";
  case ZE_RESULT_ERROR_INVALID_FUNCTION_NAME:
    return "ZE_RESULT_ERROR_INVALID_FUNCTION_NAME";
  case ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION:
    return "ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION";
  case ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION:
    return "ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION";
  case ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX:
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX";
  case ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE:
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE";
  case ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE:
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE";
  case ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED:
    return "ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED";
  case ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE:
    return "ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE";
  case ZE_RESULT_ERROR_OVERLAPPING_REGIONS:
    return "ZE_RESULT_ERROR_OVERLAPPING_REGIONS";
  case ZE_RESULT_ERROR_UNKNOWN:
    return "ZE_RESULT_ERROR_UNKNOWN";
  case ZE_RESULT_FORCE_UINT32:
    return "ZE_RESULT_FORCE_UINT32";
  default:
    return "Unknown ze_result_t value: " +
           std::to_string(static_cast<int>(result));
  }
}

std::string to_string(const ze_bool_t ze_bool) {
  if (ze_bool) {
    return "true";
  } else {
    return "false";
  }
}

std::string to_string(const ze_command_queue_flag_t flags) {
  if (flags == 0) {
    return "Default";
  } else if (flags == ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY) {
    return "ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY";
  } else if (flags == ZE_COMMAND_QUEUE_FLAG_FORCE_UINT32) {
    return "ZE_COMMAND_QUEUE_FLAG_FORCE_UINT32";
  } else {
    return "Unknown ze_command_queue_flag_t value: " +
           std::to_string(static_cast<int>(flags));
  }
}

std::string to_string(const ze_command_queue_mode_t mode) {
  if (mode == ZE_COMMAND_QUEUE_MODE_DEFAULT) {
    return "ZE_COMMAND_QUEUE_MODE_DEFAULT";
  } else if (mode == ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS) {
    return "ZE_COMMAND_QUEUE_MODE_SYNCHRONOUS";
  } else if (mode == ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS) {
    return "ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS";
  } else {
    return "Unknown ze_command_queue_mode_t value: " +
           std::to_string(static_cast<int>(mode));
  }
}

std::string to_string(const ze_command_queue_priority_t priority) {
  if (priority == ZE_COMMAND_QUEUE_PRIORITY_NORMAL) {
    return "ZE_COMMAND_QUEUE_PRIORITY_NORMAL";
  } else if (priority == ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW) {
    return "ZE_COMMAND_QUEUE_PRIORITY_LOW";
  } else if (priority == ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH) {
    return "ZE_COMMAND_QUEUE_PRIORITY_HIGH";
  } else {
    return "Unknown ze_command_queue_priority_t value: " +
           std::to_string(static_cast<int>(priority));
  }
}

std::string to_string(const ze_image_format_layout_t layout) {
  if (layout == ZE_IMAGE_FORMAT_LAYOUT_8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_16) {
    return "ZE_IMAGE_FORMAT_LAYOUT_16";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_32) {
    return "ZE_IMAGE_FORMAT_LAYOUT_32";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_8_8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_8_8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_16_16) {
    return "ZE_IMAGE_FORMAT_LAYOUT_16_16";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16) {
    return "ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_32_32) {
    return "ZE_IMAGE_FORMAT_LAYOUT_32_32";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32) {
    return "ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2) {
    return "ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_11_11_10) {
    return "ZE_IMAGE_FORMAT_LAYOUT_11_11_10";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_5_6_5) {
    return "ZE_IMAGE_FORMAT_LAYOUT_5_6_5";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1) {
    return "ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4) {
    return "ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_NV12) {
    return "ZE_IMAGE_FORMAT_LAYOUT_NV12";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_YUYV) {
    return "ZE_IMAGE_FORMAT_LAYOUT_YUYV";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_VYUY) {
    return "ZE_IMAGE_FORMAT_LAYOUT_VYUY";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_YVYU) {
    return "ZE_IMAGE_FORMAT_LAYOUT_YVYU";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_UYVY) {
    return "ZE_IMAGE_FORMAT_LAYOUT_UYVY";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_AYUV) {
    return "ZE_IMAGE_FORMAT_LAYOUT_AYUV";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_P010) {
    return "ZE_IMAGE_FORMAT_LAYOUT_P010";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y410) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y410";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_P012) {
    return "ZE_IMAGE_FORMAT_LAYOUT_P012";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y16) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y16";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_P016) {
    return "ZE_IMAGE_FORMAT_LAYOUT_P016";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y216) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y216";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_P216) {
    return "ZE_IMAGE_FORMAT_LAYOUT_P216";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_P8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_P8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_YUY2) {
    return "ZE_IMAGE_FORMAT_LAYOUT_YUY2";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_A8P8) {
    return "ZE_IMAGE_FORMAT_LAYOUT_A8P8";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_IA44) {
    return "ZE_IMAGE_FORMAT_LAYOUT_IA44";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_AI44) {
    return "ZE_IMAGE_FORMAT_LAYOUT_AI44";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y416) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y416";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_Y210) {
    return "ZE_IMAGE_FORMAT_LAYOUT_Y210";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_I420) {
    return "ZE_IMAGE_FORMAT_LAYOUT_I420";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_YV12) {
    return "ZE_IMAGE_FORMAT_LAYOUT_YV12";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_400P) {
    return "ZE_IMAGE_FORMAT_LAYOUT_400P";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_422H) {
    return "ZE_IMAGE_FORMAT_LAYOUT_422H";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_422V) {
    return "ZE_IMAGE_FORMAT_LAYOUT_422V";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_444P) {
    return "ZE_IMAGE_FORMAT_LAYOUT_444P";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_RGBP) {
    return "ZE_IMAGE_FORMAT_LAYOUT_RGBP";
  } else if (layout == ZE_IMAGE_FORMAT_LAYOUT_BRGP) {
    return "ZE_IMAGE_FORMAT_LAYOUT_BRGP";
  } else {
    return "Unknown ze_image_format_layout_t value: " +
           std::to_string(static_cast<int>(layout));
  }
}

ze_image_format_layout_t to_layout(const std::string layout) {
  if (layout == "8") {
    return ZE_IMAGE_FORMAT_LAYOUT_8;
  } else if (layout == "16") {
    return ZE_IMAGE_FORMAT_LAYOUT_16;
  } else if (layout == "32") {
    return ZE_IMAGE_FORMAT_LAYOUT_32;
  } else if (layout == "8_8") {
    return ZE_IMAGE_FORMAT_LAYOUT_8_8;
  } else if (layout == "8_8_8_8") {
    return ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
  } else if (layout == "16_16") {
    return ZE_IMAGE_FORMAT_LAYOUT_16_16;
  } else if (layout == "16_16_16_16") {
    return ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16;
  } else if (layout == "32_32") {
    return ZE_IMAGE_FORMAT_LAYOUT_32_32;
  } else if (layout == "32_32_32_32") {
    return ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
  } else if (layout == "10_10_10_2") {
    return ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2;
  } else if (layout == "11_11_10") {
    return ZE_IMAGE_FORMAT_LAYOUT_11_11_10;
  } else if (layout == "5_6_5") {
    return ZE_IMAGE_FORMAT_LAYOUT_5_6_5;
  } else if (layout == "5_5_5_1") {
    return ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1;
  } else if (layout == "4_4_4_4") {
    return ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4;
  } else if (layout == "Y8") {
    return ZE_IMAGE_FORMAT_LAYOUT_Y8;
  } else if (layout == "NV12") {
    return ZE_IMAGE_FORMAT_LAYOUT_NV12;
  } else if (layout == "YUYV") {
    return ZE_IMAGE_FORMAT_LAYOUT_YUYV;
  } else if (layout == "VYUY") {
    return ZE_IMAGE_FORMAT_LAYOUT_VYUY;
  } else if (layout == "YVYU") {
    return ZE_IMAGE_FORMAT_LAYOUT_YVYU;
  } else if (layout == "UYVY") {
    return ZE_IMAGE_FORMAT_LAYOUT_UYVY;
  } else if (layout == "AYUV") {
    return ZE_IMAGE_FORMAT_LAYOUT_AYUV;
  } else if (layout == "P010") {
    return ZE_IMAGE_FORMAT_LAYOUT_P010;
  } else if (layout == "Y410") {
    return ZE_IMAGE_FORMAT_LAYOUT_Y410;
  } else if (layout == "P012") {
    return ZE_IMAGE_FORMAT_LAYOUT_P012;
  } else if (layout == "Y16") {
    return ZE_IMAGE_FORMAT_LAYOUT_Y16;
  } else if (layout == "P016") {
    return ZE_IMAGE_FORMAT_LAYOUT_P016;
  } else if (layout == "Y216") {
    return ZE_IMAGE_FORMAT_LAYOUT_Y216;
  } else if (layout == "P216") {
    return ZE_IMAGE_FORMAT_LAYOUT_P216;
  } else {
    std::cout << "Unknown ze_image_format_layout_t value: " << layout;
    return ZE_IMAGE_FORMAT_LAYOUT_FORCE_UINT32;
  }
}

std::string to_string(const ze_image_format_type_t type) {
  if (type == ZE_IMAGE_FORMAT_TYPE_UINT) {
    return "ZE_IMAGE_FORMAT_TYPE_UINT";
  } else if (type == ZE_IMAGE_FORMAT_TYPE_SINT) {
    return "ZE_IMAGE_FORMAT_TYPE_SINT";
  } else if (type == ZE_IMAGE_FORMAT_TYPE_UNORM) {
    return "ZE_IMAGE_FORMAT_TYPE_UNORM";
  } else if (type == ZE_IMAGE_FORMAT_TYPE_SNORM) {
    return "ZE_IMAGE_FORMAT_TYPE_SNORM";
  } else if (type == ZE_IMAGE_FORMAT_TYPE_FLOAT) {
    return "ZE_IMAGE_FORMAT_TYPE_FLOAT";
  } else {
    return "Unknown ze_image_format_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

ze_image_format_type_t to_format_type(const std::string format_type) {
  if (format_type == "UINT") {
    return ZE_IMAGE_FORMAT_TYPE_UINT;
  } else if (format_type == "SINT") {
    return ZE_IMAGE_FORMAT_TYPE_SINT;
  } else if (format_type == "UNORM") {
    return ZE_IMAGE_FORMAT_TYPE_UNORM;
  } else if (format_type == "SNORM") {
    return ZE_IMAGE_FORMAT_TYPE_SNORM;
  } else if (format_type == "FLOAT") {
    return ZE_IMAGE_FORMAT_TYPE_FLOAT;
  } else {
    std::cout << "Unknown ze_image_format_type_t value: ";
    return ZE_IMAGE_FORMAT_TYPE_FORCE_UINT32;
  }
}

uint32_t num_bytes_per_pixel(const ze_image_format_layout_t layout) {
  switch (layout) {
  case ZE_IMAGE_FORMAT_LAYOUT_8:
    return 1;
  case ZE_IMAGE_FORMAT_LAYOUT_16:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_32:
    return 4;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16:
    return 4;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32:
    return 8;
  case ZE_IMAGE_FORMAT_LAYOUT_5_6_5:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_11_11_10:
    return 4;
  case ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8:
    return 4;
  case ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16:
    return 8;
  case ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32:
    return 16;
  case ZE_IMAGE_FORMAT_LAYOUT_AYUV:
    return 4;
  case ZE_IMAGE_FORMAT_LAYOUT_NV12: // 12 bits per pixel
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_P010: // 10 bits per pixel
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_P012: // 12 bits per pixel
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_P016:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_P216:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_UYVY:
  case ZE_IMAGE_FORMAT_LAYOUT_VYUY:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_Y8:
    return 1;
  case ZE_IMAGE_FORMAT_LAYOUT_Y16:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_Y216:
    return 2;
  case ZE_IMAGE_FORMAT_LAYOUT_Y410: // 10 bits per pixel
    return 2;
  default:
    LOG_ERROR << "Unrecognized image format layout: " << layout;
    return 0;
  }
}

std::string to_string(const ze_image_format_swizzle_t swizzle) {
  if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_R) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_R";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_G) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_G";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_B) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_B";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_A) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_A";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_0) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_0";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_1) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_1";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_X) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_X";
  } else if (swizzle == ZE_IMAGE_FORMAT_SWIZZLE_D) {
    return "ZE_IMAGE_FORMAT_SWIZZLE_D";
  } else {
    return "Unknown ze_image_format_swizzle_t value: " +
           std::to_string(static_cast<int>(swizzle));
  }
}

std::string to_string(const ze_image_flag_t flag) {
  std::stringstream flags;
  if (flag & ZE_IMAGE_FLAG_KERNEL_WRITE) {
    flags << "|ZE_IMAGE_FLAG_KERNEL_WRITE|";
  }
  if (flag & ZE_IMAGE_FLAG_BIAS_UNCACHED) {
    flags << "|ZE_IMAGE_FLAG_BIAS_UNCACHED|";
  }

  return to_string_helper(flags).str();
}

ze_image_flags_t to_image_flag(const std::string flag) {

  // by default setting to READ
  ze_image_flags_t image_flags = 0;

  // check if "READ" position is found in flag string
  if (flag.find("WRITE") != std::string::npos) {
    image_flags = image_flags | ZE_IMAGE_FLAG_KERNEL_WRITE;
  }
  if (flag.find("UNCACHED") != std::string::npos) {
    image_flags = image_flags | ZE_IMAGE_FLAG_BIAS_UNCACHED;
  }

  return image_flags;
}

std::string to_string(const ze_image_type_t type) {
  switch (type) {
  case ZE_IMAGE_TYPE_1D:
    return "ZE_IMAGE_TYPE_1D";
  case ZE_IMAGE_TYPE_1DARRAY:
    return "ZE_IMAGE_TYPE_1DARRAY";
  case ZE_IMAGE_TYPE_2D:
    return "ZE_IMAGE_TYPE_2D";
  case ZE_IMAGE_TYPE_2DARRAY:
    return "ZE_IMAGE_TYPE_2DARRAY";
  case ZE_IMAGE_TYPE_3D:
    return "ZE_IMAGE_TYPE_3D";
  case ZE_IMAGE_TYPE_BUFFER:
    return "ZE_IMAGE_TYPE_BUFFER";
  default:
    return "Unknown ze_image_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

ze_image_type_t to_image_type(const std::string type) {
  if (type == "1D") {
    return ZE_IMAGE_TYPE_1D;
  } else if (type == "2D") {
    return ZE_IMAGE_TYPE_2D;
  } else if (type == "3D") {
    return ZE_IMAGE_TYPE_3D;
  } else if (type == "1DARRAY") {
    return ZE_IMAGE_TYPE_1DARRAY;
  } else if (type == "2DARRAY") {
    return ZE_IMAGE_TYPE_2DARRAY;
  } else {
    std::cout << "Unknown ze_image_type_t value: ";
    return ZE_IMAGE_TYPE_FORCE_UINT32;
  }
}

std::string to_string(const ze_memory_type_t type) {
  if (type == ZE_MEMORY_TYPE_HOST) {
    return "ZE_MEMORY_TYPE_HOST";
  } else if (type == ZE_MEMORY_TYPE_DEVICE) {
    return "ZE_MEMORY_TYPE_DEVICE";
  } else if (type == ZE_MEMORY_TYPE_SHARED) {
    return "ZE_MEMORY_TYPE_SHARED";
  } else if (type == ZE_MEMORY_TYPE_HOST_IMPORTED) {
    return "ZE_MEMORY_TYPE_HOST_IMPORTED";
  } else {
    return "Unknown ze_memory_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const ze_device_fp_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_FP_FLAG_DENORM:
    return "ZE_DEVICE_FP_FLAG_DENORM";
  case ZE_DEVICE_FP_FLAG_INF_NAN:
    return "ZE_DEVICE_FP_FLAG_INF_NAN";
  case ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST:
    return "ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST";
  case ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO:
    return "ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO";
  case ZE_DEVICE_FP_FLAG_ROUND_TO_INF:
    return "ZE_DEVICE_FP_FLAG_ROUND_TO_INF";
  case ZE_DEVICE_FP_FLAG_FMA:
    return "ZE_DEVICE_FP_FLAG_FMA";
  case ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT:
    return "ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT";
  case ZE_DEVICE_FP_FLAG_SOFT_FLOAT:
    return "ZE_DEVICE_FP_FLAG_SOFT_FLOAT";
  case ZE_DEVICE_FP_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_FP_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_fp_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_memory_access_cap_flag_t flag) {
  switch (flag) {
  case ZE_MEMORY_ACCESS_CAP_FLAG_RW:
    return "ZE_MEMORY_ACCESS_CAP_FLAG_RW";
  case ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC:
    return "ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC";
  case ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT:
    return "ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT";
  case ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC:
    return "ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC";
  case ZE_MEMORY_ACCESS_CAP_FLAG_FORCE_UINT32:
    return "ZE_MEMORY_ACCESS_CAP_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_memory_access_cap_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_device_property_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_PROPERTY_FLAG_INTEGRATED:
    return "ZE_DEVICE_PROPERTY_FLAG_INTEGRATED";
  case ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE:
    return "ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE";
  case ZE_DEVICE_PROPERTY_FLAG_ECC:
    return "ZE_DEVICE_PROPERTY_FLAG_ECC";
  case ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING:
    return "ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING";
  case ZE_DEVICE_PROPERTY_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

constexpr char dec2hex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static char hexdigit(int i) { return dec2hex[i]; }
template <typename T> static void uuid_to_string(T uuid, int bytes, char *s) {
  int i;

  for (i = bytes - 1; i >= 0; --i) {
    *s++ = hexdigit(uuid.id[i] / 0x10);
    *s++ = hexdigit(uuid.id[i] % 0x10);
    if (i >= 6 && i <= 12 && (i & 1) == 0) {
      *s++ = '-';
    }
  }
  *s = '\0';
}

#define MAX_UUID_STRING_SIZE 49

std::string to_string(const ze_driver_uuid_t uuid) {
  std::ostringstream result;
  char uuid_string[MAX_UUID_STRING_SIZE];
  uuid_to_string(uuid, ZE_MAX_DRIVER_UUID_SIZE, uuid_string);
  result << uuid_string;
  return result.str();
}

std::string to_string(const ze_device_uuid_t uuid) {
  std::ostringstream result;
  char uuid_string[MAX_UUID_STRING_SIZE];
  uuid_to_string(uuid, ZE_MAX_DEVICE_UUID_SIZE, uuid_string);
  result << uuid_string;
  return result.str();
}

std::string to_string(const ze_native_kernel_uuid_t uuid) {
  std::ostringstream result;
  char uuid_string[MAX_UUID_STRING_SIZE];
  uuid_to_string(uuid, ZE_MAX_KERNEL_UUID_SIZE, uuid_string);
  result << uuid_string;
  return result.str();
}

std::string uuid_to_string(const uint8_t uuid[]) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::uppercase;
  ss << std::setw(2) << +uuid[15];
  ss << std::setw(2) << +uuid[14];
  ss << std::setw(2) << +uuid[13];
  ss << std::setw(2) << +uuid[12];
  ss << '-';
  ss << std::setw(2) << +uuid[11];
  ss << std::setw(2) << +uuid[10];
  ss << '-';
  ss << std::setw(2) << +uuid[9];
  ss << std::setw(2) << +uuid[8];
  ss << '-';
  ss << std::setw(2) << +uuid[7];
  ss << std::setw(2) << +uuid[6];
  ss << '-';
  ss << std::setw(2) << +uuid[5];
  ss << std::setw(2) << +uuid[4];
  ss << std::setw(2) << +uuid[3];
  ss << std::setw(2) << +uuid[2];
  ss << std::setw(2) << +uuid[1];
  ss << std::setw(2) << +uuid[0];
  return ss.str();
}

std::vector<std::string> split_string(const std::string &string,
                                      const std::string &delimiter) {
  if (string.empty() || delimiter.empty()) {
    return {string};
  }
  std::vector<std::string> tokens;
  size_t start = 0U;
  size_t end = string.find(delimiter);
  while (end != std::string::npos) {
    tokens.push_back(string.substr(start, end - start));
    start = end + delimiter.length();
    end = string.find(delimiter, start);
  }
  tokens.push_back(string.substr(start));
  return tokens;
}

std::string join_strings(const std::vector<std::string> &tokens,
                         const std::string &delimiter) {
  std::stringstream ss;
  for (size_t i = 0; i < tokens.size(); ++i) {
    ss << tokens[i];
    if (i < tokens.size() - 1) {
      ss << delimiter;
    }
  }
  return ss.str();
}

std::string to_string(const ze_device_type_t type) {
  switch (type) {
  case ZE_DEVICE_TYPE_GPU:
    return "ZE_DEVICE_TYPE_GPU";
  case ZE_DEVICE_TYPE_CPU:
    return "ZE_DEVICE_TYPE_CPU";
  case ZE_DEVICE_TYPE_FPGA:
    return "ZE_DEVICE_TYPE_FPGA";
  case ZE_DEVICE_TYPE_MCA:
    return "ZE_DEVICE_TYPE_MCA";
  case ZE_DEVICE_TYPE_VPU:
    return "ZE_DEVICE_TYPE_VPU";
  case ZE_DEVICE_TYPE_FORCE_UINT32:
    return "ZE_DEVICE_TYPE_FORCE_UINT32";
  default:
    return "Unknown ze_device_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zet_metric_group_sampling_type_flag_t flag) {
  switch (flag) {
  case ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED:
    return "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EVENT_BASED";
  case ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED:
    return "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_TIME_BASED";
  case ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED:
    return "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_EXP_TRACER_BASED";
  case ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_FORCE_UINT32:
    return "ZET_METRIC_GROUP_SAMPLING_TYPE_FLAG_FORCE_UINT32";
  default:
    return "Unknown zet_metric_group_sampling_type_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const zes_engine_group_t type) {
  switch (type) {
  case ZES_ENGINE_GROUP_ALL:
    return "ZES_ENGINE_GROUP_ALL";
  case ZES_ENGINE_GROUP_COMPUTE_ALL:
    return "ZES_ENGINE_GROUP_COMPUTE_ALL";
  case ZES_ENGINE_GROUP_MEDIA_ALL:
    return "ZES_ENGINE_GROUP_MEDIA_ALL";
  case ZES_ENGINE_GROUP_COPY_ALL:
    return "ZES_ENGINE_GROUP_COPY_ALL";
  case ZES_ENGINE_GROUP_COMPUTE_SINGLE:
    return "ZES_ENGINE_GROUP_COMPUTE_SINGLE";
  case ZES_ENGINE_GROUP_RENDER_SINGLE:
    return "ZES_ENGINE_GROUP_RENDER_SINGLE";
  case ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE:
    return "ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE";
  case ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE:
    return "ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE";
  case ZES_ENGINE_GROUP_COPY_SINGLE:
    return "ZES_ENGINE_GROUP_COPY_SINGLE";
  case ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE:
    return "ZES_ENGINE_GROUP_MEDIA_ENHANCEMENT_SINGLE";
  case ZES_ENGINE_GROUP_3D_SINGLE:
    return "ZES_ENGINE_GROUP_3D_SINGLE";
  case ZES_ENGINE_GROUP_3D_RENDER_COMPUTE_ALL:
    return "ZES_ENGINE_GROUP_3D_RENDER_COMPUTE_ALL";
  case ZES_ENGINE_GROUP_RENDER_ALL:
    return "ZES_ENGINE_GROUP_RENDER_ALL";
  case ZES_ENGINE_GROUP_3D_ALL:
    return "ZES_ENGINE_GROUP_3D_ALL";
  case ZES_ENGINE_GROUP_MEDIA_CODEC_SINGLE:
    return "ZES_ENGINE_GROUP_MEDIA_CODEC_SINGLE";
  case ZES_ENGINE_GROUP_FORCE_UINT32:
    return "ZES_ENGINE_GROUP_FORCE_UINT32";
  default:
    return "Unknown zes_engine_group_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_mem_type_t type) {
  switch (type) {
  case ZES_MEM_TYPE_HBM:
    return "ZES_MEM_TYPE_HBM";
  case ZES_MEM_TYPE_DDR:
    return "ZES_MEM_TYPE_DDR";
  case ZES_MEM_TYPE_DDR3:
    return "ZES_MEM_TYPE_DDR3";
  case ZES_MEM_TYPE_DDR4:
    return "ZES_MEM_TYPE_DDR4";
  case ZES_MEM_TYPE_DDR5:
    return "ZES_MEM_TYPE_DDR5";
  case ZES_MEM_TYPE_LPDDR:
    return "ZES_MEM_TYPE_LPDDR";
  case ZES_MEM_TYPE_LPDDR3:
    return "ZES_MEM_TYPE_LPDDR3";
  case ZES_MEM_TYPE_LPDDR4:
    return "ZES_MEM_TYPE_LPDDR4";
  case ZES_MEM_TYPE_LPDDR5:
    return "ZES_MEM_TYPE_LPDDR5";
  case ZES_MEM_TYPE_SRAM:
    return "ZES_MEM_TYPE_SRAM";
  case ZES_MEM_TYPE_L1:
    return "ZES_MEM_TYPE_L1";
  case ZES_MEM_TYPE_L3:
    return "ZES_MEM_TYPE_L3";
  case ZES_MEM_TYPE_GRF:
    return "ZES_MEM_TYPE_GRF";
  case ZES_MEM_TYPE_SLM:
    return "ZES_MEM_TYPE_SLM";
  case ZES_MEM_TYPE_GDDR4:
    return "ZES_MEM_TYPE_GDDR4";
  case ZES_MEM_TYPE_GDDR5:
    return "ZES_MEM_TYPE_GDDR5";
  case ZES_MEM_TYPE_GDDR5X:
    return "ZES_MEM_TYPE_GDDR5X";
  case ZES_MEM_TYPE_GDDR6:
    return "ZES_MEM_TYPE_GDDR6";
  case ZES_MEM_TYPE_GDDR6X:
    return "ZES_MEM_TYPE_GDDR6X";
  case ZES_MEM_TYPE_GDDR7:
    return "ZES_MEM_TYPE_GDDR7";
  default:
    return "Unknown zes_mem_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_mem_loc_t type) {
  switch (type) {
  case ZES_MEM_LOC_SYSTEM:
    return "ZES_MEM_LOC_SYSTEM";
  case ZES_MEM_LOC_DEVICE:
    return "ZES_MEM_LOC_DEVICE";
  default:
    return "Unknown zes_mem_loc_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_freq_domain_t type) {
  switch (type) {
  case ZES_FREQ_DOMAIN_GPU:
    return "ZES_FREQ_DOMAIN_GPU";
  case ZES_FREQ_DOMAIN_MEMORY:
    return "ZES_FREQ_DOMAIN_MEMORY";
  case ZES_FREQ_DOMAIN_MEDIA:
    return "ZES_FREQ_DOMAIN_MEDIA";
  default:
    return "Unknown zes_freq_domain_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_temp_sensors_t type) {
  switch (type) {
  case ZES_TEMP_SENSORS_GLOBAL:
    return "ZES_TEMP_SENSORS_GLOBAL";
  case ZES_TEMP_SENSORS_GPU:
    return "ZES_TEMP_SENSORS_GPU";
  case ZES_TEMP_SENSORS_MEMORY:
    return "ZES_TEMP_SENSORS_MEMORY";
  case ZES_TEMP_SENSORS_GLOBAL_MIN:
    return "ZES_TEMP_SENSORS_GLOBAL_MIN";
  case ZES_TEMP_SENSORS_GPU_MIN:
    return "ZES_TEMP_SENSORS_GPU_MIN";
  case ZES_TEMP_SENSORS_MEMORY_MIN:
    return "ZES_TEMP_SENSORS_MEMORY_MIN";
  case ZES_TEMP_SENSORS_GPU_BOARD:
    return "ZES_TEMP_SENSORS_GPU_BOARD";
  case ZES_TEMP_SENSORS_GPU_BOARD_MIN:
    return "ZES_TEMP_SENSORS_GPU_BOARD_MIN";
  case ZES_TEMP_SENSORS_VOLTAGE_REGULATOR:
    return "ZES_TEMP_SENSORS_VOLTAGE_REGULATOR";
  default:
    return "Unknown zes_temp_sensors_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_ras_error_type_t type) {
  switch (type) {
  case ZES_RAS_ERROR_TYPE_CORRECTABLE:
    return "ZES_RAS_ERROR_TYPE_CORRECTABLE";
  case ZES_RAS_ERROR_TYPE_UNCORRECTABLE:
    return "ZES_RAS_ERROR_TYPE_UNCORRECTABLE";
  default:
    return "Unknown zes_ras_error_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_standby_type_t type) {
  switch (type) {
  case ZES_STANDBY_TYPE_GLOBAL:
    return "ZES_STANDBY_TYPE_GLOBAL";
  default:
    return "Unknown zes_standby_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const zes_sched_mode_t mode) {
  switch (mode) {
  case ZES_SCHED_MODE_TIMEOUT:
    return "ZES_SCHED_MODE_TIMEOUT";
  case ZES_SCHED_MODE_TIMESLICE:
    return "ZES_SCHED_MODE_TIMESLICE";
  case ZES_SCHED_MODE_EXCLUSIVE:
    return "ZES_SCHED_MODE_EXCLUSIVE";
  case ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG:
    return "ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG";
  default:
    return "Unknown zes_sched_mode_t value: " +
           std::to_string(static_cast<int>(mode));
  }
}

std::string to_string(const zes_pci_bar_type_t type) {
  switch (type) {
  case ZES_PCI_BAR_TYPE_MMIO:
    return "ZES_PCI_BAR_TYPE_MMIO";
  case ZES_PCI_BAR_TYPE_ROM:
    return "ZES_PCI_BAR_TYPE_ROM";
  case ZES_PCI_BAR_TYPE_MEM:
    return "ZES_PCI_BAR_TYPE_MEM";
  default:
    return "Unknown zes_pci_bar_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

std::string to_string(const ze_device_memory_property_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_MEMORY_PROPERTY_FLAG_TBD:
    return "ZE_DEVICE_MEMORY_PROPERTY_FLAG_TBD";
  case ZE_DEVICE_MEMORY_PROPERTY_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_MEMORY_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_memory_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_ipc_property_flag_t flag) {
  switch (flag) {
  case ZE_IPC_PROPERTY_FLAG_MEMORY:
    return "ZE_IPC_PROPERTY_FLAG_MEMORY";
  case ZE_IPC_PROPERTY_FLAG_EVENT_POOL:
    return "ZE_IPC_PROPERTY_FLAG_EVENT_POOL";
  case ZE_IPC_PROPERTY_FLAG_FORCE_UINT32:
    return "ZE_IPC_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_ipc_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_external_memory_type_flag_t flag) {
  switch (flag) {
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_FD";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_OPAQUE_WIN32_KMT";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D11_TEXTURE_KMT";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_HEAP";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_D3D12_RESOURCE";
  case ZE_EXTERNAL_MEMORY_TYPE_FLAG_FORCE_UINT32:
    return "ZE_EXTERNAL_MEMORY_TYPE_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_external_memory_type_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_device_cache_property_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_CACHE_PROPERTY_FLAG_USER_CONTROL:
    return "ZE_DEVICE_CACHE_PROPERTY_FLAG_USER_CONTROL";
  case ZE_DEVICE_CACHE_PROPERTY_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_CACHE_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_cache_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_device_module_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_MODULE_FLAG_FP16:
    return "ZE_DEVICE_MODULE_FLAG_FP16";
  case ZE_DEVICE_MODULE_FLAG_FP64:
    return "ZE_DEVICE_MODULE_FLAG_FP64";
  case ZE_DEVICE_MODULE_FLAG_INT64_ATOMICS:
    return "ZE_DEVICE_MODULE_FLAG_INT64_ATOMICS";
  case ZE_DEVICE_MODULE_FLAG_DP4A:
    return "ZE_DEVICE_MODULE_FLAG_DP4A";
  case ZE_DEVICE_MODULE_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_MODULE_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_module_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_scheduling_hint_exp_flag_t flag) {
  switch (flag) {
  case ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST:
    return "ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST";
  case ZE_SCHEDULING_HINT_EXP_FLAG_ROUND_ROBIN:
    return "ZE_SCHEDULING_HINT_EXP_FLAG_ROUND_ROBIN";
  case ZE_SCHEDULING_HINT_EXP_FLAG_STALL_BASED_ROUND_ROBIN:
    return "ZE_SCHEDULING_HINT_EXP_FLAG_STALL_BASED_ROUND_ROBIN";
  case ZE_SCHEDULING_HINT_EXP_FLAG_FORCE_UINT32:
    return "ZE_SCHEDULING_HINT_EXP_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_scheduling_hint_exp_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_device_fp_atomic_ext_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_LOAD_STORE:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_LOAD_STORE";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_ADD:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_ADD";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_MIN_MAX:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_GLOBAL_MIN_MAX";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_LOAD_STORE:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_LOAD_STORE";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_ADD:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_ADD";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_MIN_MAX:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_LOCAL_MIN_MAX";
  case ZE_DEVICE_FP_ATOMIC_EXT_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_FP_ATOMIC_EXT_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_fp_atomic_ext_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_device_raytracing_ext_flag_t flag) {
  switch (flag) {
  case ZE_DEVICE_RAYTRACING_EXT_FLAG_RAYQUERY:
    return "ZE_DEVICE_RAYTRACING_EXT_FLAG_RAYQUERY";
  case ZE_DEVICE_RAYTRACING_EXT_FLAG_FORCE_UINT32:
    return "ZE_DEVICE_RAYTRACING_EXT_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_device_raytracing_ext_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_command_queue_group_property_flag_t flag) {
  switch (flag) {
  case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE:
    return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE";
  case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY:
    return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY";
  case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS:
    return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS";
  case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS:
    return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS";
  case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_FORCE_UINT32:
    return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_command_queue_group_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_mutable_command_list_exp_flags_t flag) {
  switch (flag) {
  case ZE_MUTABLE_COMMAND_LIST_EXP_FLAG_RESERVED:
    return "ZE_MUTABLE_COMMAND_LIST_EXP_FLAG_RESERVED";
  case ZE_MUTABLE_COMMAND_LIST_EXP_FLAG_FORCE_UINT32:
    return "ZE_MUTABLE_COMMAND_LIST_EXP_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_mutable_command_list_exp_flags_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const ze_mutable_command_exp_flag_t flag) {
  switch (flag) {
  case ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS";
  case ZE_MUTABLE_COMMAND_EXP_FLAG_FORCE_UINT32:
    return "ZE_MUTABLE_COMMAND_EXP_FLAG_FORCE_UINT32";
  default:
    return "Unknown ze_mutable_command_exp_flags_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const zet_device_debug_property_flag_t flag) {
  switch (flag) {
  case ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH:
    return "ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH";
  case ZET_DEVICE_DEBUG_PROPERTY_FLAG_FORCE_UINT32:
    return "ZET_DEVICE_DEBUG_PROPERTY_FLAG_FORCE_UINT32";
  default:
    return "Unknown zet_device_debug_property_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

std::string to_string(const zes_engine_type_flag_t flag) {
  switch (flag) {
  case ZES_ENGINE_TYPE_FLAG_OTHER:
    return "ZES_ENGINE_TYPE_FLAG_OTHER";
  case ZES_ENGINE_TYPE_FLAG_COMPUTE:
    return "ZES_ENGINE_TYPE_FLAG_COMPUTE";
  case ZES_ENGINE_TYPE_FLAG_3D:
    return "ZES_ENGINE_TYPE_FLAG_3D";
  case ZES_ENGINE_TYPE_FLAG_MEDIA:
    return "ZES_ENGINE_TYPE_FLAG_MEDIA";
  case ZES_ENGINE_TYPE_FLAG_DMA:
    return "ZES_ENGINE_TYPE_FLAG_DMA";
  case ZES_ENGINE_TYPE_FLAG_RENDER:
    return "ZES_ENGINE_TYPE_FLAG_RENDER";
  default:
    return "Unknown zes_engine_type_flag_t value: " +
           std::to_string(static_cast<int>(flag));
  }
}

} // namespace level_zero_tests

std::ostream &operator<<(std::ostream &os, const ze_api_version_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_result_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_bool_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_command_queue_flag_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_command_queue_mode_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os,
                         const ze_command_queue_priority_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_image_format_layout_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_image_format_type_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_image_format_swizzle_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_image_flag_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_image_type_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_device_fp_flag_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_driver_uuid_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_native_kernel_uuid_t &x) {
  return os << level_zero_tests::to_string(x);
}

std::ostream &operator<<(std::ostream &os, const ze_device_uuid_t &x) {
  return os << level_zero_tests::to_string(x);
}

bool operator==(const ze_device_uuid_t &id_a, const ze_device_uuid_t &id_b) {
  return !(memcmp(id_a.id, id_b.id, ZE_MAX_DEVICE_UUID_SIZE));
}

bool operator!=(const ze_device_uuid_t &id_a, const ze_device_uuid_t &id_b) {
  return memcmp(id_a.id, id_b.id, ZE_MAX_DEVICE_UUID_SIZE);
}

bool operator==(const ze_device_thread_t &a, const ze_device_thread_t &b) {

  return ((a.slice == b.slice) && (a.subslice == b.subslice) &&
          (a.eu == b.eu) && (a.thread == b.thread));
}
bool operator!=(const ze_device_thread_t &a, const ze_device_thread_t &b) {

  return ((a.slice != b.slice) || (a.subslice != b.subslice) ||
          (a.eu != b.eu) || (a.thread != b.thread));
}