/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_P2P_COMMON_HPP
#define level_zero_tests_ZE_TEST_P2P_COMMON_HPP

typedef enum _lzt_p2p_memory_type_tests {
  LZT_P2P_MEMORY_TYPE_DEVICE =
      0, ///< P2P test the memory pointed to is a device allocation
  LZT_P2P_MEMORY_TYPE_SHARED =
      1, ///< P2P test the memory pointed to is a shared allocation
  LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION =
      2 ///< P2P test the memory pointed to is a reserved allocation
} lzt_p2p_memory_type_tests_t;

#endif