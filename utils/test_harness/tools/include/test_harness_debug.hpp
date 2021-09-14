/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_DEBUG_HPP
#define TEST_HARNESS_DEBUG_HPP

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

zet_device_debug_properties_t get_debug_properties(ze_device_handle_t device);

zet_debug_session_handle_t debug_attach(const ze_device_handle_t &device,
                                        const zet_debug_config_t &debug_config);

void debug_detach(const zet_debug_session_handle_t &debug_session);

zet_debug_event_t
debug_read_event(const zet_debug_session_handle_t &debug_session,
                 uint64_t timeout);

void debug_ack_event(const zet_debug_session_handle_t &debug_session,
                     const zet_debug_event_t *debug_event);

void debug_interrupt(const zet_debug_session_handle_t &debug_session,
                     const ze_device_thread_t &device_thread);

void debug_resume(const zet_debug_session_handle_t &debug_session,
                  const ze_device_thread_t &device_thread);

void debug_read_memory(const zet_debug_session_handle_t &debug_session,
                       const ze_device_thread_t &device_thread,
                       const zet_debug_memory_space_desc_t desc, size_t size,
                       void *buffer);
void debug_write_memory(const zet_debug_session_handle_t &debug_session,
                        const ze_device_thread_t &device_thread,
                        const zet_debug_memory_space_desc_t desc, size_t size,
                        const void *buffer);

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_DEBUG_HPP */
