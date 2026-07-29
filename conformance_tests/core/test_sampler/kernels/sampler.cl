/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

const sampler_t sampler_adr_none_filter_nearest_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_none_filter_nearest_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_none_filter_linear_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_none_filter_linear_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_repeat_filter_nearest_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_repeat_filter_nearest_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_repeat_filter_linear_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_repeat_filter_linear_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_clamp_filter_nearest_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_clamp_filter_nearest_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE |
    CLK_FILTER_NEAREST;
const sampler_t sampler_adr_clamp_filter_linear_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_clamp_filter_linear_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_mirror_filter_nearest_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_MIRRORED_REPEAT |
    CLK_FILTER_NEAREST;
const sampler_t sampler_adr_mirror_filter_nearest_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_MIRRORED_REPEAT |
    CLK_FILTER_NEAREST;
const sampler_t sampler_adr_mirror_filter_linear_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_MIRRORED_REPEAT |
    CLK_FILTER_LINEAR;
const sampler_t sampler_adr_mirror_filter_linear_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_MIRRORED_REPEAT |
    CLK_FILTER_LINEAR;
const sampler_t sampler_adr_clamp_to_border_filter_nearest_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_clamp_to_border_filter_nearest_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t sampler_adr_clamp_to_border_filter_linear_normalized =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
const sampler_t sampler_adr_clamp_to_border_filter_linear_unnormalized =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

kernel void sampler_inkernel_adr_none_filter_nearest_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_none_filter_nearest_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_none_filter_nearest_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_none_filter_nearest_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_none_filter_linear_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_none_filter_linear_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_none_filter_linear_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_none_filter_linear_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_repeat_filter_nearest_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_repeat_filter_nearest_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_repeat_filter_nearest_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_repeat_filter_nearest_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_repeat_filter_linear_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_repeat_filter_linear_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_repeat_filter_linear_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_repeat_filter_linear_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_filter_nearest_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_filter_nearest_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_filter_nearest_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_filter_nearest_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_filter_linear_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_filter_linear_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_filter_linear_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_filter_linear_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_mirror_filter_nearest_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_mirror_filter_nearest_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_mirror_filter_nearest_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_mirror_filter_nearest_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_mirror_filter_linear_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_mirror_filter_linear_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_mirror_filter_linear_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_mirror_filter_linear_unnormalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_to_border_filter_nearest_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel =
      read_imagef(input, sampler_adr_clamp_to_border_filter_nearest_normalized,
                  input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_to_border_filter_nearest_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_to_border_filter_nearest_unnormalized,
      input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_to_border_filter_linear_normalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(
      input, sampler_adr_clamp_to_border_filter_linear_normalized, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inkernel_adr_clamp_to_border_filter_linear_unnormalized(
    read_only image2d_t input, write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel =
      read_imagef(input, sampler_adr_clamp_to_border_filter_linear_unnormalized,
                  input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_inhost(read_only image2d_t input,
                           write_only image2d_t output,
                           sampler_t sampler_host) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_host, input_coord);
  write_imagef(output, output_coord, pixel);
}

kernel void sampler_noop(sampler_t sampler) {}
