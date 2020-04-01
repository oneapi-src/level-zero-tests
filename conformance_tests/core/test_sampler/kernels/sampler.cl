/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/* Note:
      If these samplers are updated the corresponding test
      case should be updated in test_sampler.cpp
*/
const sampler_t sampler_kernel0 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel1 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel2 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_NONE | CLK_FILTER_LINEAR;
const sampler_t sampler_kernel3 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_LINEAR;

const sampler_t sampler_kernel4 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel5 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel6 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
const sampler_t sampler_kernel7 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;

const sampler_t sampler_kernel8 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel9 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel10 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;
const sampler_t sampler_kernel11 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;

const sampler_t sampler_kernel12 = CLK_NORMALIZED_COORDS_TRUE |
                                   CLK_ADDRESS_MIRRORED_REPEAT |
                                   CLK_FILTER_NEAREST;
const sampler_t sampler_kernel13 = CLK_NORMALIZED_COORDS_FALSE |
                                   CLK_ADDRESS_MIRRORED_REPEAT |
                                   CLK_FILTER_NEAREST;
const sampler_t sampler_kernel14 = CLK_NORMALIZED_COORDS_TRUE |
                                   CLK_ADDRESS_MIRRORED_REPEAT |
                                   CLK_FILTER_LINEAR;
const sampler_t sampler_kernel15 = CLK_NORMALIZED_COORDS_FALSE |
                                   CLK_ADDRESS_MIRRORED_REPEAT |
                                   CLK_FILTER_LINEAR;

const sampler_t sampler_kernel16 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
const sampler_t sampler_kernel17 = CLK_NORMALIZED_COORDS_FALSE |
                                   CLK_ADDRESS_CLAMP_TO_EDGE |
                                   CLK_FILTER_NEAREST;
const sampler_t sampler_kernel18 =
    CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t sampler_kernel19 =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

kernel void sampler_inkernel0(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel0, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel1(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel1, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel2(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel2, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel3(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel3, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel4(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel4, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel5(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel5, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel6(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel6, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel7(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel7, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel8(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel8, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel9(read_only image2d_t input,
                              write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel9, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel10(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel10, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel11(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel11, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel12(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel12, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel13(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel13, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel14(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel14, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel15(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel15, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel16(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel16, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel17(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel17, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel18(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel18, input_coord);
  write_imagef(output, output_coord, pixel);
}
kernel void sampler_inkernel19(read_only image2d_t input,
                               write_only image2d_t output) {
  const int2 output_coord = {get_global_id(0), get_global_id(1)};
  const int2 output_size = {get_global_size(0), get_global_size(1)};

  const float pixel_center = 0.5f;
  const float2 input_coord = (convert_float2(output_coord) + pixel_center) /
                             convert_float2(output_size);
  const float4 pixel = read_imagef(input, sampler_kernel19, input_coord);
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
