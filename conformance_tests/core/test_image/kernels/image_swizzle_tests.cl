/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t image_sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

kernel void swizzle_test_img_1d(read_only image1d_t image_in,
                                write_only image1d_t image_out) {

  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}

kernel void swizzle_test_img_1d_arr(read_only image1d_array_t image_in,
                                    write_only image1d_array_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}

kernel void swizzle_test_img_2d(read_only image2d_t image_in,
                                write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}

kernel void swizzle_test_img_2d_arr(read_only image2d_array_t image_in,
                                    write_only image2d_array_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}

kernel void swizzle_test_img_3d(read_only image3d_t image_in,
                                write_only image3d_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  write_imageui(image_out, coord, pixel);
}