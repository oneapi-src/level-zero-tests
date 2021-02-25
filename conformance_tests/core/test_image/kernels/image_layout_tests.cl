/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t image_sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;



kernel void copy_float_image(read_only image3d_t image_in, write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  write_imagef(image_out, coord, pixel);
}

kernel void copy_uint_image(read_only image3d_t image_in, write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}

kernel void copy_sint_image(read_only image3d_t image_in, write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  int4 pixel = read_imagei(image_in, image_sampler, coord);
  write_imagei(image_out, coord, pixel);
}