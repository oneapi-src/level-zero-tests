/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
struct node {
  uint value;
  global struct node *next;
};

kernel void residency_function(global struct node *node_data, int size) {
  const int tid = get_global_id(0);

  global struct node *item = node_data;
  if (tid == 0) {
    item = node_data;
    for (int i = 0; i < size + 1; i++) {
      item->value = item->value + 1;
      item = item->next;
    }
  }
}
