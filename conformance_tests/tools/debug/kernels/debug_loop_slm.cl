/*
 *
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
kernel void long_kernel_slm(global uchar *dst, global uchar *src,
                        global ulong *loop_count, ulong max, global uchar *slmOut) {
  uint gid = get_global_id(0);
  uint gsize = get_global_size(0);
  int mid = gsize / 2;
  __local unsigned char memLocal[512];

  if (gid < 512){
    memLocal[gid] = src[gid];
  }

  ulong loop = 0;
  while (loop < max && src[0] != 0x00) {
    dst[gid] = (char)(gid + 2 & 0xff);
    loop++;
  }
  dst[gid] = src[gid];

  if (gid < 512){
    slmOut[gid] = memLocal[gid];
  }

  *loop_count = loop;

  if ((gid == 0) || (gid == mid) || (gid == (gsize - 1))) {
    printf("[GPU kernel] Gid:%d src[%d]=%d size=%d , after looping %lu out of "
           "%lu\n",
           gid, gid, src[gid], gsize, loop, max);
     
    if (gid ==0 ) {
      printf("[GPU Kernel] SLM Buffer address %p size: 512\n", &memLocal[0]);
    }
  }
}