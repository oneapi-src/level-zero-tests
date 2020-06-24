/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAXITERATION 50

float get_color(const uint iterations, const uint max_iteration) {
  float pixel = 0.0f;
  if (iterations != max_iteration) {
    const float factor = sqrt((float)iterations / (float)max_iteration);
    const float raw_value = round((float)max_iteration * factor);
    pixel = (raw_value / (float)max_iteration);
  }
  return pixel;
}

kernel void mandelbrot(global float *pixels, int width, int height) {
  const uint x = get_global_id(0);
  const uint y = get_global_id(1);

  float2 uv = {(float)x / (float)width, (float)y / (float)height};
  float2 params = {1.75f, 1.25f};
  float2 c = (uv * 2.5f) - params;
  float2 z = 0.0f;
  
  int i;
  
  for (i = 0; i < MAXITERATION; ++i) {
    if (dot(z, z) > 2*2) {
      break;
    }
    const float tmp = z.x * z.x - z.y * z.y + c.x;
    z.y = 2 * z.x * z.y + c.y;
    z.x = tmp;
  }
  const int iterations = i;

  const uint pixel_offset = y * width + x;
  pixels[pixel_offset] = get_color(iterations, MAXITERATION);
}
