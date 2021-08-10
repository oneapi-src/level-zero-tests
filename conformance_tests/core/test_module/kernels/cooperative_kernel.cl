/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// compute the n'th row of Pascal's triangle and store in buffer
kernel void module_cooperative_pascal(global ulong *buffer, const int n) {

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

  ulong val;
  for (int i = 0; i <= n; i++) {
    if (xid == 0 || xid == i) {
      val = 1;
    } else if (xid < i) {
      val = buffer[xid - 1] + buffer[xid];
    }
    // Adding the global barriers should guarantee not to deadlock
    // because the cooperative kernel API will suggest a group count
    // such that all groups are active
    global_barrier();
    buffer[xid] = val;
    global_barrier(); // ensure that all threads have finished
    // updating previous row before continuing
  }
}
