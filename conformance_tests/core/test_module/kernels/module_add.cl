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

kernel void module_add_constant_2(global uchar *values, int addval) {

  const size_t xid = get_global_id(0);
  const size_t yid = get_global_id(1);
  const size_t zid = get_global_id(2);
  const size_t xdim = get_global_size(0);
  const size_t ydim = get_global_size(1);
  const size_t zdim = get_global_size(2);

  values[xid] = values[xid] + addval;
}

kernel void module_add_two_arrays(global int *output, global int *input) {
  const int tid = get_global_id(0);
  output[tid] = output[tid] + input[tid];
}

kernel __attribute__((work_group_size_hint(1, 1, 1))) void
module_add_attr(global int *output, global int *input) {
  const int tid = get_global_id(0);
  output[tid] = output[tid] + input[tid];
}
