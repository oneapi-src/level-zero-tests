/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t image_sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

kernel void image_format_uint(read_only image2d_t image_in,
                              write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_int(read_only image2d_t image_in,
                             write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void image_format_float(read_only image2d_t image_in,
                               write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_unorm(read_only image2d_t image_in,
                               write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_snorm(read_only image2d_t image_in,
                               write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_one_component(read_only image2d_t image_in,
                                              write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 1;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_two_components(read_only image2d_t image_in,
                                               write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 1;
  pixel.y += 1;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_three_components_unorm(read_only image2d_t image_in,
                                           write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_three_components_float(read_only image2d_t image_in,
                                           write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_four_components(read_only image2d_t image_in,
                                    write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 1;
  pixel.y += 1;
  pixel.z += 1;
  pixel.w += 1;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_four_components_unorm(read_only image2d_t image_in,
                                          write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}