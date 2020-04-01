/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void barrier_add_constant(global int *values, int addval) {

  const int xid = get_global_id(0);
  values[xid] = values[xid] + addval;
}

kernel void barrier_add_two_arrays(global int *output, global int *input) {
  const int xid = get_global_id(0);
  output[xid] = output[xid] + input[xid];
}
