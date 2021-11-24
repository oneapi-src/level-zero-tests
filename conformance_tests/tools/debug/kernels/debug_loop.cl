/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void debug_print_forever() {

  const int xid = get_global_id(0);
  const int yid = get_global_id(1);
  const int zid = get_global_id(2);
  const int xdim = get_global_size(0);
  const int ydim = get_global_size(1);
  const int zdim = get_global_size(2);

  uint count = 0;
  while (1) {
    count++;

    if (!(count % 500)) {
      printf("debug print\n");
    }
  }
}
