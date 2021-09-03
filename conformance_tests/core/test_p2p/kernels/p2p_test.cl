/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void multi_device_function(global char *data, int offset) {
  const int tid = get_global_id(0);

  if (tid == 0) {
    data[offset]++;
  }
}
