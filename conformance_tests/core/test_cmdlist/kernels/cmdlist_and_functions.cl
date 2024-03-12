/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void test_printf() {
  uint number = 1234;
  printf("printf test_printf message only %d\n", number);
}

kernel void test_get_global_id( __global uint *out) {
  const uint xid = get_global_id(0);
  out[xid] = xid;
}

kernel void test_get_group_id( __global uint *out) {
  const uint xid = get_group_id(0);
  out[xid] = xid;
}
