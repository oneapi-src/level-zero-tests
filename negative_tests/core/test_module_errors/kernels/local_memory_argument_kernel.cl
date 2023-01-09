/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */


kernel void single_local(__local char* local1, __global char* global1){
  const int id = get_global_id(0);
  if(id>0)
      return;

  local1[0] = 0x55;
  global1[0] = local1[0];
}
