/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void profile_add_constant(global int *values_in, global int *values_out, int addval) {

  const int xid = get_global_id(0);
  values_out[xid] = values_in[xid] + addval;
}
