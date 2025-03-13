/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t image_sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

/* ONE COMPONENT */

kernel void
image_format_layout_1_component_uint_img_1d(read_only image1d_t image_in,
                                            write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_1_component_uint_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_1_component_uint_img_2d(read_only image2d_t image_in,
                                            write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_1_component_uint_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_1_component_uint_img_3d(read_only image3d_t image_in,
                                            write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  write_imageui(image_out, coord, pixel);
}

/* TWO component */

kernel void
image_format_layout_2_component_uint_img_1d(read_only image1d_t image_in,
                                            write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_2_component_uint_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_2_component_uint_img_2d(read_only image2d_t image_in,
                                            write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_2_component_uint_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_2_component_uint_img_3d(read_only image3d_t image_in,
                                            write_only image3d_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  write_imageui(image_out, coord, pixel);
}

/* THREE component */

/* UNORM */

kernel void
image_format_layout_3_component_unorm_img_1d(read_only image1d_t image_in,
                                             write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_3_component_unorm_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_3_component_unorm_img_2d(read_only image2d_t image_in,
                                             write_only image2d_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_3_component_unorm_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_3_component_unorm_img_3d(read_only image3d_t image_in,
                                             write_only image3d_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

/* FLOAT */

kernel void
image_format_layout_3_component_float_img_1d(read_only image1d_t image_in,
                                             write_only image1d_t image_out) {

  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_3_component_float_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {

  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_3_component_float_img_2d(read_only image2d_t image_in,
                                             write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_3_component_float_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_3_component_float_img_3d(read_only image3d_t image_in,
                                             write_only image3d_t image_out) {

  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  write_imagef(image_out, coord, pixel);
}

/* FOUR component */

/* UINT */

kernel void
image_format_layout_4_component_uint_img_1d(read_only image1d_t image_in,
                                            write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  pixel.z += 2;
  pixel.w += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_4_component_uint_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  pixel.z += 2;
  pixel.w += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_4_component_uint_img_2d(read_only image2d_t image_in,
                                            write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  pixel.z += 2;
  pixel.w += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_layout_4_component_uint_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  pixel.z += 2;
  pixel.w += 2;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_layout_4_component_uint_img_3d(read_only image3d_t image_in,
                                            write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, image_sampler, coord);
  pixel.x += 2;
  pixel.y += 2;
  pixel.z += 2;
  pixel.w += 2;
  write_imageui(image_out, coord, pixel);
}

/* UNORM */

kernel void
image_format_layout_4_component_unorm_img_1d(read_only image1d_t image_in,
                                             write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_4_component_unorm_img_1d_arr(
    read_only image1d_array_t image_in, write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_4_component_unorm_img_2d(read_only image2d_t image_in,
                                             write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_layout_4_component_unorm_img_2d_arr(
    read_only image2d_array_t image_in, write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_layout_4_component_unorm_img_3d(read_only image3d_t image_in,
                                             write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 0.5f;
  pixel.y += 0.5f;
  pixel.z += 0.5f;
  pixel.w += 0.5f;
  write_imagef(image_out, coord, pixel);
}