/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void double_values(global uint *data) {
  const int i = get_global_id(0);
  data[i] = data[i] * 2;
}
