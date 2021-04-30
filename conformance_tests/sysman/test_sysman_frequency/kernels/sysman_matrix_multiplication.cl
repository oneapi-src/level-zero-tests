/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// Matrix A has M rows and K columns
// Matrix B has K rows and N columns
// Matrix C has M rows and N columns

#define TILE_SIZE 16
__attribute__((reqd_work_group_size(TILE_SIZE, TILE_SIZE, 1))) kernel void
sysman_matrix_multiplication(const global float *a, const global float *b,
                         const int m, const int k, const int n,
                         global float *c) {
  const int2 global_id = {get_global_id(0), get_global_id(1)};
  const int2 local_id = {get_local_id(0), get_local_id(1)};

  local float a_tile[TILE_SIZE * TILE_SIZE];
  local float b_tile[TILE_SIZE * TILE_SIZE];

  float sum = 0.0f;
  for (int tile_id = 0; tile_id < k / TILE_SIZE; ++tile_id) {
    a_tile[local_id.y * TILE_SIZE + local_id.x] =
        a[(tile_id * TILE_SIZE + local_id.y) * m + global_id.x];
    b_tile[local_id.y * TILE_SIZE + local_id.x] =
        b[global_id.y * k + (tile_id * TILE_SIZE + local_id.x)];
    barrier(CLK_LOCAL_MEM_FENCE);

    for (int i = 0; i < TILE_SIZE; ++i) {
      sum += a_tile[i * TILE_SIZE + local_id.x] *
             b_tile[local_id.y * TILE_SIZE + i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
  }

  c[global_id.y * m + global_id.x] = sum;
}
