/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
kernel void long_kernel(global uchar *dst, global uchar *src) {
  uint gid = get_global_id(0);
  bool exit_flag = false;

  ulong max = 99900000;
  ulong loop = 0;
  while (loop < max && src[0] != 0x00) {
    dst[gid] = (char)(gid + 2 & 0xff);
    loop++;
  }
  dst[gid] = src[gid];

  if (gid == 0) {
    printf(
        "[GPU kernel] Gid:%d Hello src[gid] %d, after looping %lu out of %lu\n",
        gid, src[gid], loop, max);
  }
}
