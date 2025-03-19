/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t image_sampler =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

/* ZE_IMAGE_TYPE_1D */

kernel void image_format_uint_img_1d(read_only image1d_t image_in,
                                     write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_sint_img_1d(read_only image1d_t image_in,
                                     write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void image_format_float_img_1d(read_only image1d_t image_in,
                                      write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_unorm_img_1d(read_only image1d_t image_in,
                                      write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_snorm_img_1d(read_only image1d_t image_in,
                                      write_only image1d_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

/* ZE_IMAGE_TYPE_1DARRAY */

kernel void image_format_uint_img_1d_arr(read_only image1d_array_t image_in,
                                         write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_sint_img_1d_arr(read_only image1d_array_t image_in,
                                         write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void
image_format_float_img_1d_arr(read_only image1d_array_t image_in,
                              write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_unorm_img_1d_arr(read_only image1d_array_t image_in,
                              write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_snorm_img_1d_arr(read_only image1d_array_t image_in,
                              write_only image1d_array_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

/* ZE_IMAGE_TYPE_2D */

kernel void image_format_uint_img_2d(read_only image2d_t image_in,
                                     write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_sint_img_2d(read_only image2d_t image_in,
                                     write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void image_format_float_img_2d(read_only image2d_t image_in,
                                      write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_unorm_img_2d(read_only image2d_t image_in,
                                      write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_snorm_img_2d(read_only image2d_t image_in,
                                      write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

/* ZE_IMAGE_TYPE_2DARRAY */

kernel void image_format_uint_img_2d_arr(read_only image2d_array_t image_in,
                                         write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_sint_img_2d_arr(read_only image2d_array_t image_in,
                                         write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void
image_format_float_img_2d_arr(read_only image2d_array_t image_in,
                              write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_unorm_img_2d_arr(read_only image2d_array_t image_in,
                              write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_snorm_img_2d_arr(read_only image2d_array_t image_in,
                              write_only image2d_array_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

/* ZE_IMAGE_TYPE_3D */

kernel void image_format_uint_img_3d(read_only image3d_t image_in,
                                     write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void image_format_sint_img_3d(read_only image3d_t image_in,
                                     write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void image_format_float_img_3d(read_only image3d_t image_in,
                                      write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_unorm_img_3d(read_only image3d_t image_in,
                                      write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void image_format_snorm_img_3d(read_only image3d_t image_in,
                                      write_only image3d_t image_out) {
  const int4 coord = {get_global_id(0), get_global_id(1), get_global_id(2), 0};
  float4 pixel = read_imagef(image_in, image_sampler, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}

/* ZE_IMAGE_TYPE_BUFFER */

kernel void
image_format_uint_img_buffer(read_only image1d_buffer_t image_in,
                                       write_only image1d_buffer_t image_out) {
  const int coord = {get_global_id(0)};
  uint4 pixel = read_imageui(image_in, coord);
  pixel.x++;
  write_imageui(image_out, coord, pixel);
}

kernel void
image_format_sint_img_buffer(read_only image1d_buffer_t image_in,
                                       write_only image1d_buffer_t image_out) {
  const int coord = {get_global_id(0)};
  int4 pixel = read_imagei(image_in, coord);
  pixel.x--;
  write_imagei(image_out, coord, pixel);
}

kernel void
image_format_float_img_buffer(read_only image1d_buffer_t image_in,
                                        write_only image1d_buffer_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, coord);
  pixel.x -= 0.123f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_unorm_img_buffer(read_only image1d_buffer_t image_in,
                                        write_only image1d_buffer_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, coord);
  pixel.x += 1.0f;
  write_imagef(image_out, coord, pixel);
}

kernel void
image_format_snorm_img_buffer(read_only image1d_buffer_t image_in,
                                        write_only image1d_buffer_t image_out) {
  const int coord = {get_global_id(0)};
  float4 pixel = read_imagef(image_in, coord);
  pixel.x += 2.0f;
  write_imagef(image_out, coord, pixel);
}