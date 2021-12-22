/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_HARNESS_DEBUG_HPP
#define TEST_HARNESS_DEBUG_HPP

#include <boost/process.hpp>

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

zet_device_debug_properties_t get_debug_properties(ze_device_handle_t device);

zet_debug_session_handle_t debug_attach(const ze_device_handle_t &device,
                                        const zet_debug_config_t &debug_config);

void debug_detach(const zet_debug_session_handle_t &debug_session);

ze_result_t debug_read_event(const zet_debug_session_handle_t &debug_session,
                             zet_debug_event_t &debugEvent, uint64_t timeout,
                             bool allowTimeout);

void debug_ack_event(const zet_debug_session_handle_t &debug_session,
                     const zet_debug_event_t *debug_event);

void debug_interrupt(const zet_debug_session_handle_t &debug_session,
                     const ze_device_thread_t &device_thread);

void debug_resume(const zet_debug_session_handle_t &debug_session,
                  const ze_device_thread_t &device_thread);

void debug_read_memory(const zet_debug_session_handle_t &debug_session,
                       const ze_device_thread_t &device_thread,
                       const zet_debug_memory_space_desc_t &desc, size_t size,
                       void *buffer);

void debug_write_memory(const zet_debug_session_handle_t &debug_session,
                        const ze_device_thread_t &device_thread,
                        const zet_debug_memory_space_desc_t &desc, size_t size,
                        const void *buffer);

uint32_t get_register_set_properties_count(const ze_device_handle_t &device);

std::vector<zet_debug_regset_properties_t>
get_register_set_properties(const ze_device_handle_t &device);

void debug_clean_assert_true(bool condition,
                             boost::process::child &debug_helper);

}; // namespace level_zero_tests

#endif /* TEST_HARNESS_DEBUG_HPP */
