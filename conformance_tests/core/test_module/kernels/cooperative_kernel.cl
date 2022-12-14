/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void cooperative_kernel(global ulong *buffer, const int n) {

  const int work_group_id = get_group_id(0);
  const int xid = get_global_id(0);
  const int xsize = get_global_size(0);
  const int xgsize = get_local_size(0);
  const int work_group_thread_id = get_local_id(0);

  if (xid == 0) {
    printf("[wgid_%d_wgtid_%d xid_%d] Number of work-items total: %d\n",
           work_group_id, work_group_thread_id, xid, xsize);
    printf("Number of items in work-group: %d\n", xgsize);
  }

  ulong val = 0;
  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
  for (int i = 0; i <= n; i++) {
    val = xid + n;
    buffer[xid] = val;
  }
  barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
}
