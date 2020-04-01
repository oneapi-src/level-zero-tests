/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "cl_image_copy.h"

using namespace std;

cl_channel_type ClImageCopy::to_channel_datatype(std::string channel_datatype) {
  if (channel_datatype == "SNORM_INT8") {
    return CL_SNORM_INT8;
  } else if (channel_datatype == "SNORM_INT16") {
    return CL_SNORM_INT16;
  } else if (channel_datatype == "UNORM_INT8") {
    return CL_UNORM_INT8;
  } else if (channel_datatype == "UNORM_INT16") {
    return CL_UNORM_INT16;
  } else if (channel_datatype == "UNORM_SHORT_565") {
    return CL_UNORM_SHORT_565;
  } else if (channel_datatype == "UNORM_SHORT_555") {
    return CL_UNORM_SHORT_555;
  } else if (channel_datatype == "UNORM_INT_101010") {
    return CL_UNORM_INT_101010;
  } else if (channel_datatype == "SIGNED_INT8") {
    return CL_SIGNED_INT8;
  } else if (channel_datatype == "SIGNED_INT16") {
    return CL_SIGNED_INT16;
  } else if (channel_datatype == "SIGNED_INT32") {
    return CL_SIGNED_INT32;
  } else if (channel_datatype == "UNSIGNED_INT8") {
    return CL_UNSIGNED_INT8;
  } else if (channel_datatype == "UNSIGNED_INT16") {
    return CL_UNSIGNED_INT16;
  } else if (channel_datatype == "UNSIGNED_INT32") {
    return CL_UNSIGNED_INT32;
  } else if (channel_datatype == "HALF_FLOAT") {
    return CL_HALF_FLOAT;
  } else if (channel_datatype == "FLOAT") {
    return CL_FLOAT;
  }
  // No match found
  abort();
}
/////////

cl_channel_order ClImageCopy::to_channel_order(std::string channel_order) {
  if (channel_order == "R") {
    return CL_R;
  } else if (channel_order == "A") {
    return CL_A;
  } else if (channel_order == "INTENSITY") {
    return CL_INTENSITY;
  } else if (channel_order == "LUMINANCE") {
    return CL_LUMINANCE;
  } else if (channel_order == "RG") {
    return CL_RG;
  } else if (channel_order == "RA") {
    return CL_RA;
  } else if (channel_order == "RGB") {
    return CL_RGB;
  } else if (channel_order == "RGBA") {
    return CL_RGBA;
  } else if (channel_order == "ARGB") {
    return CL_ARGB;
  } else if (channel_order == "BGRA") {
    return CL_BGRA;
  }
  // No match found
  abort();
}

cl_mem_object_type ClImageCopy::to_image_type(std::string image_type) {
  if (image_type == "BUFFER") {
    return CL_MEM_OBJECT_BUFFER;
  } else if (image_type == "2D") {
    return CL_MEM_OBJECT_IMAGE2D;
  } else if (image_type == "3D") {
    return CL_MEM_OBJECT_IMAGE3D;
  } else if (image_type == "2DARRAY") {
    return CL_MEM_OBJECT_IMAGE2D_ARRAY;
  } else if (image_type == "1D") {
    return CL_MEM_OBJECT_IMAGE1D;
  } else if (image_type == "1DARRAY") {
    return CL_MEM_OBJECT_IMAGE1D_ARRAY;
  } else if (image_type == "1DBUFFER") {
    return CL_MEM_OBJECT_IMAGE1D_BUFFER;
  }
  // No match found
  abort();
}

namespace level_zero_tests {

// for channel order, channel type and mem_object type
std::string to_string(const cl_uint cl_var) {
  if (cl_var == CL_SNORM_INT8) {
    return "CL_SNORM_INT8";
  } else if (cl_var == CL_SNORM_INT16) {
    return "SNORM_INT16";
  } else if (cl_var == CL_UNORM_INT8) {
    return "CL_UNORM_INT8";
  } else if (cl_var == CL_UNORM_INT16) {
    return "CL_UNORM_INT16";
  } else if (cl_var == CL_UNORM_SHORT_565) {
    return "CL_UNORM_SHORT_565";
  } else if (cl_var == CL_UNORM_SHORT_555) {
    return "CL_UNORM_SHORT_555";
  } else if (cl_var == CL_UNORM_INT_101010) {
    return "CL_UNORM_INT_101010";
  } else if (cl_var == CL_SIGNED_INT8) {
    return "CL_SIGNED_INT8";
  } else if (cl_var == CL_SIGNED_INT16) {
    return "CL_SIGNED_INT16";
  } else if (cl_var == CL_SIGNED_INT32) {
    return "CL_SIGNED_INT32";
  } else if (cl_var == CL_UNSIGNED_INT8) {
    return "CL_UNSIGNED_INT8";
  } else if (cl_var == CL_UNSIGNED_INT16) {
    return "CL_UNSIGNED_INT16";
  } else if (cl_var == CL_UNSIGNED_INT32) {
    return "CL_UNSIGNED_INT32";
  } else if (cl_var == CL_HALF_FLOAT) {
    return "CL_HALF_FLOAT";
  } else if (cl_var == CL_FLOAT) {
    return "CL_FLOAT";
  } else if (cl_var == CL_A) {
    return "CL_A";
  } else if (cl_var == CL_R) {
    return "CL_R";
  } else if (cl_var == CL_RG) {
    return "CL_RG";
  } else if (cl_var == CL_RA) {
    return "CL_RA";
  } else if (cl_var == CL_RGB) {
    return "CL_RGB";
  } else if (cl_var == CL_RGBA) {
    return "CL_RGBA";
  } else if (cl_var == CL_BGRA) {
    return "CL_BGRA";
  } else if (cl_var == CL_ARGB) {
    return "CL_ARGB";
  } else if (cl_var == CL_INTENSITY) {
    return "CL_INTENSITY";
  } else if (cl_var == CL_LUMINANCE) {
    return "CL_LUMINANCE";
  } else if (cl_var == CL_Rx) {
    return "CL_Rx";
  } else if (cl_var == CL_RGx) {
    return "CL_RGx";
  } else if (cl_var == CL_RGBx) {
    return "CL_RGBx";
  } else if (cl_var == CL_MEM_OBJECT_BUFFER) {
    return "CL_MEM_OBJECT_BUFFER";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE2D) {
    return "CL_MEM_OBJECT_IMAGE2D";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE3D) {
    return "CL_MEM_OBJECT_IMAGE3D";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE2D_ARRAY) {
    return "CL_MEM_OBJECT_IMAGE2D_ARRAY";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE1D) {
    return "CL_MEM_OBJECT_IMAGE1D";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE1D_ARRAY) {
    return "CL_MEM_OBJECT_IMAGE1D_ARRAY";
  } else if (cl_var == CL_MEM_OBJECT_IMAGE1D_BUFFER) {
    return "CL_MEM_OBJECT_IMAGE1D_BUFFER";
  } else {
    return "Unknown cl_image_format_type_t value: " +
           std::to_string(static_cast<int>(cl_var));
  }
}
} // namespace level_zero_tests
