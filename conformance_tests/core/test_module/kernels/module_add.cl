/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void module_add_constant(global int *values, int addval) {

  const int xid = get_global_id(0);
  const int yid = get_global_id(1);
  const int zid = get_global_id(2);
  const int xdim = get_global_size(0);
  const int ydim = get_global_size(1);
  const int zdim = get_global_size(2);
  atomic_add(&values[0], addval);
  if (((xid + 1) == xdim) && ((yid + 1) == ydim) && ((zid + 1) == zdim)) {
    printf("values[0] = %7d xid = %3d  yid = %3d  zid = %3d\n", values[0], xid,
           yid, zid);
  }
}

kernel void module_add_two_arrays(global int *output, global int *input) {
  const int tid = get_global_id(0);
  output[tid] = output[tid] + input[tid];
  printf("output[%3d] = %5d\n", tid, output[tid]);
}

kernel  __attribute__(( work_group_size_hint(1, 1, 1) ))
void module_add_attr (global int *output, global int *input) {
  const int tid = get_global_id(0);
  output[tid] = output[tid] + input[tid];
}

