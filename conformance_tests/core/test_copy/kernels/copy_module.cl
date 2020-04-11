/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void copy_data(global int *input_buffer, global int *output_buffer,
                      int offset, int size) {
  // copy data from input_buffer to output_buffer starting
  // at offset

  const int xid = get_global_id(0);

  if (xid == 0) {
    for (int i = 0; i < size - offset; i++) {
      output_buffer[i + offset] = input_buffer[i];
    }
  }
}

struct copy_data {
  uint *data;
};

kernel void copy_data_indirect(global struct copy_data *input_buffer,
                               global struct copy_data *output_buffer,
                               int offset, int size) {

  const int xid = get_global_id(0);

  for (int i = 0; i < size - offset; i++) {
    output_buffer[xid].data[i + offset] = input_buffer[xid].data[i];
  }
}
