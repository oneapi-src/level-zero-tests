/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void bitonic_sort(global int *a, const int p, const int q) {
  const int i = get_global_id(0);
  const int d = (1 << (p - q));

  if ((i & d) == 0) {
    const int up = ((i >> p) & 2) == 0;
    const int a1 = a[i];
    const int a2 = a[i | d];

    if ((a1 > a2) == up) {
      a[i] = a2;
      a[i | d] = a1;
    }
  }
}
