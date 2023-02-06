/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void transform_kernel(global uchar *values, int device_idx) {

  const int xid = get_global_id(0);
  const int yid = get_global_id(1);
  const int zid = get_global_id(2);
  const int xdim = get_global_size(0);
  const int ydim = get_global_size(1);
  const int zdim = get_global_size(2);

  values[xid] = values[xid] * 3 + 11 * (device_idx + 1);
}
