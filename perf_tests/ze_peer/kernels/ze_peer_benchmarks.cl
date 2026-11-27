/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void copy_peer(__global const uint4 *src, __global uint4 *dst,
                        ulong size) {
  const ulong count = size / sizeof(uint4);
  for (ulong i = get_global_id(0); i < count; i += get_global_size(0)) {
    dst[i] = src[i];
  }

  if (get_global_id(0) == 0) {
    __global const uchar *src_bytes = (__global const uchar *)src;
    __global uchar *dst_bytes = (__global uchar *)dst;
    for (ulong i = count * sizeof(uint4); i < size; i++) {
      dst_bytes[i] = src_bytes[i];
    }
  }
}
