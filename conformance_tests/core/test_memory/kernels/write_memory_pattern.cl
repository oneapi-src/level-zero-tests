/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void write_memory_pattern(global char *data, int size) {
  char dp = -1;
  for (int i = 0; i < size; i++) {
    data[i] = dp;
    dp = (dp - 1) & 0xff;
  }
}
