/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void cmdlist_add_constant(global int *values, int addval) {

  const int xid = get_global_id(0);
  values[xid] = values[xid] + addval;
}
