/*
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (C) 2019 Elias
 *
 * SPDX-License-Identifier: MIT
 *
 */
__kernel __attribute__((reqd_work_group_size(1, 1, 1))) void
kPingPong(__global int *buf) {
  if (get_global_id(0) == 0)
    (*buf)++;
}
