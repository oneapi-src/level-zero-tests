/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void multi_device_function(global char *data) {
  const int tid = get_global_id(0);
  data[tid]++;
}
