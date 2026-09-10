/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// https://github.com/boostorg/beast/issues/1895

#ifndef LZT_WINDOWS_WINSOCK_GUARD_H
#define LZT_WINDOWS_WINSOCK_GUARD_H

#if defined(_WIN32) || defined(_WIN64)

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#endif
