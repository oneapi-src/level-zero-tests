/*
 *
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <iostream>
#include <fstream>

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
  if (result == ZE_RESULT_SUCCESS) {
    return "ZE_RESULT_SUCCESS";
  } else if (result == ZE_RESULT_NOT_READY) {
    return "ZE_RESULT_NOT_READY";
  } else if (result == ZE_RESULT_ERROR_UNINITIALIZED) {
    return "ZE_RESULT_ERROR_UNINITIALIZED";
  } else if (result == ZE_RESULT_ERROR_DEVICE_LOST) {
    return "ZE_RESULT_ERROR_DEVICE_LOST";
  } else if (result == ZE_RESULT_ERROR_INVALID_ARGUMENT) {
    return "ZE_RESULT_ERROR_INVALID_ARGUMENT";
  } else if (result == ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY) {
    return "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY";
  } else if (result == ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY) {
    return "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY";
  } else if (result == ZE_RESULT_ERROR_MODULE_BUILD_FAILURE) {
    return "ZE_RESULT_ERROR_MODULE_BUILD_FAILURE";
  } else if (result == ZE_RESULT_ERROR_MODULE_LINK_FAILURE) {
    return "ZE_RESULT_ERROR_MODULE_LINK_FAILURE";
  } else if (result == ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS) {
    return "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS";
  } else if (result == ZE_RESULT_ERROR_NOT_AVAILABLE) {
    return "ZE_RESULT_ERROR_NOT_AVAILABLE";
  } else if (result == ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE) {
    return "ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE";
  } else if (result == ZE_RESULT_WARNING_DROPPED_DATA) {
    return "ZE_RESULT_WARNING_DROPPED_DATA";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_VERSION) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_VERSION";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
  } else if (result == ZE_RESULT_ERROR_INVALID_NULL_HANDLE) {
    return "ZE_RESULT_ERROR_INVALID_NULL_HANDLE";
  } else if (result == ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE) {
    return "ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE";
  } else if (result == ZE_RESULT_ERROR_INVALID_NULL_POINTER) {
    return "ZE_RESULT_ERROR_INVALID_NULL_POINTER";
  } else if (result == ZE_RESULT_ERROR_INVALID_SIZE) {
    return "ZE_RESULT_ERROR_INVALID_SIZE";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_SIZE) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_SIZE";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT";
  } else if (result == ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT) {
    return "ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT";
  } else if (result == ZE_RESULT_ERROR_INVALID_ENUMERATION) {
    return "ZE_RESULT_ERROR_INVALID_ENUMERATION";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION";
  } else if (result == ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT) {
    return "ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT";
  } else if (result == ZE_RESULT_ERROR_INVALID_NATIVE_BINARY) {
    return "ZE_RESULT_ERROR_INVALID_NATIVE_BINARY";
  } else if (result == ZE_RESULT_ERROR_INVALID_GLOBAL_NAME) {
    return "ZE_RESULT_ERROR_INVALID_GLOBAL_NAME";
  } else if (result == ZE_RESULT_ERROR_INVALID_KERNEL_NAME) {
    return "ZE_RESULT_ERROR_INVALID_KERNEL_NAME";
  } else if (result == ZE_RESULT_ERROR_INVALID_FUNCTION_NAME) {
    return "ZE_RESULT_ERROR_INVALID_FUNCTION_NAME";
  } else if (result == ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION) {
    return "ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION";
  } else if (result == ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION) {
    return "ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION";
  } else if (result == ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX) {
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX";
  } else if (result == ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE) {
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE";
  } else if (result == ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE) {
    return "ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE";
  } else if (result == ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED) {
    return "ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED";
  } else if (result == ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE) {
    return "ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE";
  } else if (result == ZE_RESULT_ERROR_OVERLAPPING_REGIONS) {
    return "ZE_RESULT_ERROR_OVERLAPPING_REGIONS";
  } else if (result == ZE_RESULT_ERROR_UNKNOWN) {
    return "ZE_RESULT_ERROR_UNKNOWN";
  } else {
    throw std::runtime_error("Unknown ze_result_t value: " +
                             std::to_string(static_cast<int>(result)));
  }
}

std::string to_string(const ze_bool_t ze_bool) {
  if (ze_bool) {
    return "True";
  } else {
    return "False";
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

uint32_t num_bytes_per_pixel(ze_image_format_layout_t layout) {
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

std::string to_string(const ze_device_fp_flag_t flags) {
  std::stringstream bitfield;
  if (flags & ZE_DEVICE_FP_FLAG_DENORM) {
    bitfield << "|ZE_DEVICE_FP_FLAG_DENORM|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_INF_NAN) {
    bitfield << "|ZE_DEVICE_FP_FLAG_INF_NAN|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST) {
    bitfield << "|ZE_DEVICE_FP_FLAG_ROUND_TO_NEAREST|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO) {
    bitfield << "|ZE_DEVICE_FP_FLAG_ROUND_TO_ZERO|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_ROUND_TO_INF) {
    bitfield << "|ZE_DEVICE_FP_FLAG_ROUND_TO_INF|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_FMA) {
    bitfield << "|ZE_DEVICE_FP_FLAG_FMA|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT) {
    bitfield << "|ZE_DEVICE_FP_FLAG_ROUNDED_DIVIDE_SQRT|";
  }
  if (flags & ZE_DEVICE_FP_FLAG_SOFT_FLOAT) {
    bitfield << "|ZE_DEVICE_FP_FLAG_SOFT_FLOAT|";
  }
  return to_string_helper(bitfield).str();
}

std::string to_string(const ze_memory_access_cap_flag_t flags) {
  std::stringstream bitfield;
  if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_RW) {
    bitfield << "|ZE_MEMORY_ACCESS_CAP_FLAG_RW|";
  }
  if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC) {
    bitfield << "|ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC|";
  }
  if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT) {
    bitfield << "|ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT|";
  }
  if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC) {
    bitfield << "|ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC|";
  }
  if (flags & ZE_MEMORY_ACCESS_CAP_FLAG_FORCE_UINT32) {
    bitfield << "|ZE_MEMORY_ACCESS_CAP_FLAG_FORCE_UINT32|";
  }
  return to_string_helper(bitfield).str();
}

std::string to_string(const ze_device_property_flag_t flags) {
  std::stringstream bitfield;
  if (flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) {
    bitfield << "|ZE_DEVICE_PROPERTY_FLAG_INTEGRATED|";
  }
  if (flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
    bitfield << "|ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE|";
  }
  if (flags & ZE_DEVICE_PROPERTY_FLAG_ECC) {
    bitfield << "|ZE_DEVICE_PROPERTY_FLAG_ECC|";
  }
  if (flags & ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
    bitfield << "|ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING|";
  }
  if (flags & ZE_DEVICE_PROPERTY_FLAG_FORCE_UINT32) {
    bitfield << "|ZE_DEVICE_PROPERTY_FLAG_FORCE_UINT32|";
  }
  return to_string_helper(bitfield).str();
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