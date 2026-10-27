/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void double_image(read_only image2d_t image_in,
                         write_only image2d_t image_out) {
  const int2 coord = {get_global_id(0), get_global_id(1)};
  write_imageui(image_out, coord, read_imageui(image_in, coord) * 2);
}
