/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
kernel void long_kernel(global uchar *dst, global uchar *src,
                        global ulong *loop_count, ulong max) {
  uint gid = get_global_id(0);
  uint gsize = get_global_size(0);
  int mid = gsize / 2;

  ulong loop = 0;
  while (loop < max && src[0] != 0x00) {
    dst[gid] = (char)(gid + 2 & 0xff);
    loop++;
  }
  dst[gid] = src[gid];

  *loop_count = loop;

  if ((gid == 0) || (gid == mid) || (gid == (gsize - 1))) {
    printf("[GPU kernel] Gid:%d src[%d]=%d size=%d , after looping %lu out of "
           "%lu\n",
           gid, gid, src[gid], gsize, loop, max);
  }
}