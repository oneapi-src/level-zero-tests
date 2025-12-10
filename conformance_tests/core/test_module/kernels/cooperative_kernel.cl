/*
 *
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void cooperative_kernel(global int *buffer) {

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
  
  buffer[work_group_id] = work_group_id;

  global_barrier();

  for (int i = 0; i < get_num_groups(0); ++i) {
    if (buffer[i] != i) {
        buffer[work_group_id] = -1;
    }
  }
}
