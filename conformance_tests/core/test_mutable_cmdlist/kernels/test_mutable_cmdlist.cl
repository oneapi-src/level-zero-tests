/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void addValue(global int *inOut, int val) {
  const int gId = get_global_id(0);
  inOut[gId] += val;
}

kernel void subValue(global int *inOut, int val) {
  const int gId = get_global_id(0);
  inOut[gId] -= val;
}

kernel void mulValue(global int *inOut, int val) {
  const int gId = get_global_id(0);
  inOut[gId] *= val;
}

kernel void divValue(global int *inOut, int val) {
  const int gId = get_global_id(0);
  inOut[gId] /= val;
}

kernel void testGlobalSizes(global int *inOut,
                            global int *globalSizes) {
  int gIdX = get_global_id(0);
  int gIdY = get_global_id(1);
  int gIdZ = get_global_id(2);
  int gSizeX = get_global_size(0);
  int gSizeY = get_global_size(1);
  int gSizeZ = get_global_size(2);

  if (gIdX == 0 && gIdY == 0 && gIdZ == 0) {
    globalSizes[0] = gSizeX;
    globalSizes[1] = gSizeY;
    globalSizes[2] = gSizeZ;
  }
  int linearIndex = gIdZ * gSizeY * gSizeX + gIdY * gSizeX + gIdX;
  inOut[linearIndex] += linearIndex;
}

kernel void testLocalSizes(global int *inOut,
                           global int *localSizes) {
  int lIdX = get_local_id(0);
  int lIdY = get_local_id(1);
  int lIdZ = get_local_id(2);
  int lSizeX = get_local_size(0);
  int lSizeY = get_local_size(1);
  int lSizeZ = get_local_size(2);
  int groupIdX = get_group_id(0);
  int groupIdY = get_group_id(1);
  int groupIdZ = get_group_id(2);
  int groupNumX = get_num_groups(0);
  int groupNumY = get_num_groups(1);
  int groupNumZ = get_num_groups(2);

  if (lIdX == 0 && lIdY == 0 && lIdZ == 0 && groupIdX == 0 && groupIdY == 0 && groupIdZ == 0) {
    localSizes[0] = lSizeX;
    localSizes[1] = lSizeY;
    localSizes[2] = lSizeZ;
  }
  int globalOffset = (groupIdZ * groupNumY * groupNumX + groupIdY * groupNumX + groupIdX) * lSizeX * lSizeY * lSizeZ;
  int localOffset = lIdZ * lSizeY * lSizeX + lIdY * lSizeX + lIdX;
  int linearIndex = globalOffset + localOffset;
  inOut[linearIndex] += linearIndex;
}

kernel void testGlobalOffset(global int *globalOffsets) {
  int gIdX = get_global_id(0);
  int gIdY = get_global_id(1);
  int gIdZ = get_global_id(2);

  if (gIdX == 1 && gIdY == 2 && gIdZ == 3) {
    globalOffsets[0] = get_global_offset(0);
    globalOffsets[1] = get_global_offset(1);
    globalOffsets[2] = get_global_offset(2);
  }
  if (gIdX == 4 && gIdY == 5 && gIdZ == 6) {
    globalOffsets[0] += get_global_offset(0);
    globalOffsets[1] += get_global_offset(1);
    globalOffsets[2] += get_global_offset(2);
  }
}