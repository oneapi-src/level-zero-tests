/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */


kernel void many_buffers(global int *buf1, global int *buf2, global int *buf3, global int *buf4, global int *buf5) {
    
    const int id = get_global_id(0);
    if(id>0)
        return;
    *buf1=1;
    *buf2=2;
    *buf3=3;
    *buf4=4;
    *buf5=5;
}

kernel void many_locals(__local char* local1, __local char* local2, __local char* local3, __local char* local4, __global char* global1){
  const int id = get_global_id(0);
  if(id>0)
      return;
  const int size = 256;
  for(int i=0; i< size; i++){
    local1[i] = 0x55;
    local2[i] = local1[i];
    local3[i] = local2[i];
    local4[i] = local3[i];
    global1[i] = local4[i];
  }
}
