/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_CMDLIST_HPP
#define level_zero_tests_ZE_TEST_HARNESS_CMDLIST_HPP

#include "test_harness_device.hpp"
#include <level_zero/ze_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

struct zeCommandBundle {
  ze_command_queue_handle_t queue = nullptr;
  ze_command_list_handle_t list = nullptr;
};

ze_command_list_handle_t create_command_list(ze_context_handle_t context,
                                             ze_device_handle_t device,
                                             ze_command_list_flags_t flags,
                                             uint32_t ordinal);
ze_command_list_handle_t create_command_list(ze_context_handle_t context,
                                             ze_device_handle_t device,
                                             ze_command_list_flags_t flags);
ze_command_list_handle_t create_command_list(ze_device_handle_t device,
                                             ze_command_list_flags_t flags);
ze_command_list_handle_t create_command_list(ze_context_handle_t context,
                                             ze_device_handle_t device);
ze_command_list_handle_t create_command_list(ze_device_handle_t device);
ze_command_list_handle_t create_command_list();

ze_command_list_handle_t create_immediate_command_list(
    ze_device_handle_t device, ze_command_queue_flags_t flags,
    ze_command_queue_mode_t, ze_command_queue_priority_t priority,
    uint32_t ordinal);
ze_command_list_handle_t
create_immediate_command_list(ze_device_handle_t device);

ze_command_list_handle_t create_immediate_command_list(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t flags, ze_command_queue_mode_t mode,
    ze_command_queue_priority_t priority, uint32_t ordinal, uint32_t index);
ze_command_list_handle_t create_immediate_command_list(
    ze_context_handle_t context, ze_device_handle_t device, const void *pNext,
    ze_command_queue_flags_t flags, ze_command_queue_mode_t mode,
    ze_command_queue_priority_t priority, uint32_t ordinal, uint32_t index);

ze_command_list_handle_t create_immediate_command_list();

void append_command_lists_immediate_exp(
    ze_command_list_handle_t hCommandList, uint32_t numCommandLists,
    ze_command_list_handle_t *phCommandLists, ze_event_handle_t hSignalEvent);
void append_command_lists_immediate_exp(
    ze_command_list_handle_t hCommandList, uint32_t numCommandLists,
    ze_command_list_handle_t *phCommandLists);
void append_command_lists_immediate_exp(
    ze_command_list_handle_t hCommandList, uint32_t numCommandLists,
    ze_command_list_handle_t *phCommandLists, ze_event_handle_t hSignalEvent,
    uint32_t numWaitEvents, ze_event_handle_t *phWaitEvents);

zeCommandBundle create_command_bundle(bool isImmediate);
zeCommandBundle create_command_bundle(ze_device_handle_t device,
                                      bool isImmediate);
zeCommandBundle create_command_bundle(ze_device_handle_t device,
                                      ze_command_list_flags_t listFlags,
                                      bool isImmediate);
zeCommandBundle create_command_bundle(ze_context_handle_t context,
                                      ze_device_handle_t device,
                                      bool isImmediate);
zeCommandBundle create_command_bundle(ze_context_handle_t context,
                                      ze_device_handle_t device,
                                      ze_command_list_flags_t listFlags,
                                      bool isImmediate);
zeCommandBundle create_command_bundle(ze_context_handle_t context,
                                      ze_device_handle_t device,
                                      ze_command_list_flags_t listFlags,
                                      uint32_t ordinal, bool isImmediate);
zeCommandBundle create_command_bundle(ze_device_handle_t device,
                                      ze_command_queue_flags_t queueFlags,
                                      ze_command_queue_mode_t mode,
                                      ze_command_queue_priority_t priority,
                                      ze_command_list_flags_t listFlags,
                                      uint32_t ordinal, bool isImmediate);
zeCommandBundle create_command_bundle(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t queueFlags, ze_command_queue_mode_t mode,
    ze_command_queue_priority_t priority, ze_command_list_flags_t listFlags,
    uint32_t ordinal, uint32_t index, bool isImmediate);

void append_memory_advise(ze_command_list_handle_t cl,
                          ze_device_handle_t device, const void *ptr,
                          size_t size, ze_memory_advice_t advice);

void append_memory_set(ze_command_list_handle_t cl, void *dstptr,
                       const uint8_t *value, size_t size);
void append_memory_set(ze_command_list_handle_t cl, void *dstptr,
                       const uint8_t *value, size_t size,
                       ze_event_handle_t hSignalEvent);

void append_memory_fill(ze_command_list_handle_t cl, void *dstptr,
                        const void *pattern, size_t pattern_size, size_t size,
                        ze_event_handle_t hSignalEvent);
void append_memory_fill(ze_command_list_handle_t cl, void *dstptr,
                        const void *pattern, size_t pattern_size, size_t size,
                        ze_event_handle_t hSignalEvent,
                        uint32_t num_wait_events,
                        ze_event_handle_t *wait_events);

void append_memory_copy(ze_command_list_handle_t cl, void *dstptr,
                        const void *srcptr, size_t size,
                        ze_event_handle_t hSignalEvent);
void append_memory_copy(ze_command_list_handle_t cl, void *dstptr,
                        const void *srcptr, size_t size);
void append_memory_copy(ze_command_list_handle_t cl, void *dstptr,
                        const void *srcptr, size_t size,
                        ze_event_handle_t hSignalEvent,
                        uint32_t num_wait_events,
                        ze_event_handle_t *wait_events);
void append_memory_copy(ze_context_handle_t src_context,
                        ze_command_list_handle_t cl, void *dstptr,
                        const void *srcptr, size_t size,
                        ze_event_handle_t hSignalEvent,
                        uint32_t num_wait_events,
                        ze_event_handle_t *wait_events);

void append_memory_copy_region(ze_command_list_handle_t hCommandList,
                               void *dstptr, const ze_copy_region_t *dstRegion,
                               uint32_t dstPitch, uint32_t dstSlicePitch,
                               const void *srcptr,
                               const ze_copy_region_t *srcRegion,
                               uint32_t srcPitch, uint32_t srcSlicePitch,
                               ze_event_handle_t hSignalEvent);
void append_memory_copy_region(ze_command_list_handle_t hCommandList,
                               void *dstptr, const ze_copy_region_t *dstRegion,
                               uint32_t dstPitch, uint32_t dstSlicePitch,
                               const void *srcptr,
                               const ze_copy_region_t *srcRegion,
                               uint32_t srcPitch, uint32_t srcSlicePitch,
                               ze_event_handle_t hSignalEvent,
                               uint32_t num_wait_events,
                               ze_event_handle_t *wait_events);

void append_barrier(ze_command_list_handle_t cl,
                    ze_event_handle_t hSignalEvent);
void append_barrier(ze_command_list_handle_t cl, ze_event_handle_t hSignalEvent,
                    uint32_t numWaitEvents, ze_event_handle_t *phWaitEvents);
void append_barrier(ze_command_list_handle_t cl);

void append_memory_ranges_barrier(ze_command_list_handle_t hCommandList,
                                  uint32_t numRanges, const size_t *pRangeSizes,
                                  const void **pRanges,
                                  ze_event_handle_t hSignalEvent,
                                  uint32_t numWaitEvents,
                                  ze_event_handle_t *phWaitEvents);

void append_launch_function(ze_command_list_handle_t hCommandList,
                            ze_kernel_handle_t hFunction,
                            const ze_group_count_t *pLaunchFuncArgs,
                            ze_event_handle_t hSignalEvent,
                            uint32_t numWaitEvents,
                            ze_event_handle_t *phWaitEvents);
void append_launch_cooperative_function(ze_command_list_handle_t hCommandList,
                                        ze_kernel_handle_t hFunction,
                                        const ze_group_count_t *pLaunchFuncArgs,
                                        ze_event_handle_t hSignalEvent,
                                        uint32_t numWaitEvents,
                                        ze_event_handle_t *phWaitEvents);
void append_signal_event(ze_command_list_handle_t hCommandList,
                         ze_event_handle_t hEvent);
void append_wait_on_events(ze_command_list_handle_t hCommandList,
                           uint32_t numEvents, ze_event_handle_t *phEvents);
void query_event(ze_event_handle_t event,
                 ze_result_t result = ZE_RESULT_SUCCESS);
void append_reset_event(ze_command_list_handle_t hCommandList,
                        ze_event_handle_t hEvent);

void append_image_copy(ze_command_list_handle_t hCommandList,
                       ze_image_handle_t dst, ze_image_handle_t src,
                       ze_event_handle_t hEvent);
void append_image_copy(ze_command_list_handle_t hCommandList,
                       ze_image_handle_t dst, ze_image_handle_t src,
                       ze_event_handle_t hEvent, uint32_t num_wait_events,
                       ze_event_handle_t *wait_events);

void append_image_copy_to_mem_ext(ze_command_list_handle_t hCommandList,
                                  void *dst, ze_image_handle_t src,
                                  uint32_t destRowPitch,
                                  uint32_t destSlicePitch,
                                  ze_event_handle_t hEvent);
void append_image_copy_to_mem_ext(ze_command_list_handle_t hCommandList,
                                  void *dst, ze_image_handle_t src,
                                  ze_image_region_t region,
                                  uint32_t destRowPitch,
                                  uint32_t destSlicePitch,
                                  ze_event_handle_t hEvent);
void append_image_copy_to_mem_ext(
    ze_command_list_handle_t hCommandList, void *dst, ze_image_handle_t src,
    uint32_t destRowPitch, uint32_t destSlicePitch, ze_event_handle_t hEvent,
    uint32_t num_wait_events, ze_event_handle_t *wait_events);
void append_image_copy_to_mem_ext(
    ze_command_list_handle_t hCommandList, void *dst, ze_image_handle_t src,
    ze_image_region_t region, uint32_t destRowPitch, uint32_t destSlicePitch,
    ze_event_handle_t hEvent, uint32_t num_wait_events,
    ze_event_handle_t *wait_events);

void append_image_copy_from_mem_ext(ze_command_list_handle_t hCommandList,
                                    ze_image_handle_t dst, void *src,
                                    uint32_t srcRowPitch,
                                    uint32_t srcSlicePitch,
                                    ze_event_handle_t hEvent);
void append_image_copy_from_mem_ext(ze_command_list_handle_t hCommandList,
                                    ze_image_handle_t dst, void *src,
                                    ze_image_region_t region,
                                    uint32_t srcRowPitch,
                                    uint32_t srcSlicePitch,
                                    ze_event_handle_t hEvent);
void append_image_copy_from_mem_ext(
    ze_command_list_handle_t hCommandList, ze_image_handle_t dst, void *src,
    uint32_t srcRowPitch, uint32_t srcSlicePitch, ze_event_handle_t hEvent,
    uint32_t num_wait_events, ze_event_handle_t *wait_events);
void append_image_copy_from_mem_ext(
    ze_command_list_handle_t hCommandList, ze_image_handle_t dst, void *src,
    ze_image_region_t region, uint32_t srcRowPitch, uint32_t srcSlicePitch,
    ze_event_handle_t hEvent, uint32_t num_wait_events,
    ze_event_handle_t *wait_events);

void append_image_copy_from_mem(ze_command_list_handle_t hCommandList,
                                ze_image_handle_t dst, void *src,
                                ze_event_handle_t hEvent);
void append_image_copy_from_mem(ze_command_list_handle_t hCommandList,
                                ze_image_handle_t dst, void *src,
                                ze_image_region_t region,
                                ze_event_handle_t hEvent);
void append_image_copy_from_mem(ze_command_list_handle_t hCommandList,
                                ze_image_handle_t dst, void *src,
                                ze_image_region_t region,
                                ze_event_handle_t hEvent,
                                uint32_t num_wait_events,
                                ze_event_handle_t *wait_events);
void append_image_copy_from_mem(ze_command_list_handle_t hCommandList,
                                ze_image_handle_t dst, void *src,
                                ze_event_handle_t hEvent,
                                uint32_t num_wait_events,
                                ze_event_handle_t *wait_events);

void append_image_copy_to_mem(ze_command_list_handle_t hCommandList, void *dst,
                              ze_image_handle_t src, ze_image_region_t region,
                              ze_event_handle_t hEvent);
void append_image_copy_to_mem(ze_command_list_handle_t hCommandList, void *dst,
                              ze_image_handle_t src, ze_image_region_t region,
                              ze_event_handle_t hEvent,
                              uint32_t num_wait_events,
                              ze_event_handle_t *wait_events);

void append_image_copy_to_mem(ze_command_list_handle_t hCommandList, void *dst,
                              ze_image_handle_t src, ze_event_handle_t hEvent);
void append_image_copy_to_mem(ze_command_list_handle_t hCommandList, void *dst,
                              ze_image_handle_t src, ze_event_handle_t hEvent,
                              uint32_t num_wait_events,
                              ze_event_handle_t *wait_events);

void append_image_copy_region(ze_command_list_handle_t hCommandList,
                              ze_image_handle_t dst, ze_image_handle_t src,
                              const ze_image_region_t *dst_region,
                              const ze_image_region_t *src_region,
                              ze_event_handle_t hEvent);
void append_image_copy_region(ze_command_list_handle_t hCommandList,
                              ze_image_handle_t dst, ze_image_handle_t src,
                              const ze_image_region_t *dst_region,
                              const ze_image_region_t *src_region,
                              ze_event_handle_t hEvent,
                              uint32_t num_wait_events,
                              ze_event_handle_t *wait_events);

void synchronize_command_list_host(ze_command_list_handle_t cl,
                                   uint64_t timeout);

void close_command_list(ze_command_list_handle_t cl);
void reset_command_list(ze_command_list_handle_t cl);
void destroy_command_list(ze_command_list_handle_t cl);

void execute_and_sync_command_bundle(zeCommandBundle bundle, uint64_t timeout);
void destroy_command_bundle(zeCommandBundle bundle);

}; // namespace level_zero_tests
#endif
