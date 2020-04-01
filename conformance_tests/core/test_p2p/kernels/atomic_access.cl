/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void atomic_access(volatile global int *output, int addval) {

  const int tid = get_global_id(0);
  const int output_size = get_global_size(0);

  atomic_add(&output[0], addval);
  if ((tid + 1) == output_size) {
    printf("global_id = %d  global_size = %d  output[0] = %d\n", tid,
           output_size, output[0]);
  }
}
