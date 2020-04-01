/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void unified_mem_test(global char *data, int size) {
  int i, errCnt = 0;
  char dp = 1;

  /* First, read the data buffer, and verify it has the expected data pattern.
     Count the errors found: */
  for (i = 0; i < size; i++) {
    if (data[i] != dp)
      errCnt++;
    dp = (dp + 1) & 0xff;
  }
  /* If we successfully validated the data buffer then write a new data pattern
     back. The caller of this function will confirm the following data pattern
     after the
     the device function completes, to ensure validation was correct. */
  if (errCnt == 0) {
    dp = -1;
    for (i = 0; i < size; i++) {
      data[i] = dp;
      dp = (dp - 1) & 0xff;
    }
  }
}
