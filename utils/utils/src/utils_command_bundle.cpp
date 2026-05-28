/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils_command_bundle.hpp"

#include <cstdlib>
#include <utility>

namespace level_zero_tests {

namespace {

template <command_list_mode_t Mode>
command_bundle to_runtime(command_bundle_t<Mode> b) {
  command_bundle r;
  if constexpr (command_bundle_t<Mode>::has_queue) {
    r.queue = b.queue;
  }
  r.list = b.list;
  if constexpr (command_bundle_t<Mode>::has_append_list) {
    r.append_list = b.append_list;
  }
  r.mode = Mode;
  return r;
}

template <command_list_mode_t Mode>
command_bundle_t<Mode> to_typed(const command_bundle &b) {
  command_bundle_t<Mode> t;
  if constexpr (command_bundle_t<Mode>::has_queue) {
    t.queue = b.queue;
  }
  t.list = b.list;
  if constexpr (command_bundle_t<Mode>::has_append_list) {
    t.append_list = b.append_list;
  }
  return t;
}

template <typename F>
decltype(auto) dispatch_mode(command_list_mode_t mode, F &&f) {
  switch (mode) {
  case command_list_mode_t::regular:
    return f.template operator()<command_list_mode_t::regular>();
  case command_list_mode_t::immediate:
    return f.template operator()<command_list_mode_t::immediate>();
  case command_list_mode_t::immediate_append_regular:
    return f
        .template operator()<command_list_mode_t::immediate_append_regular>();
  }
  std::abort();
}

} // namespace

command_bundle create_command_bundle(command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>());
  });
}

command_bundle create_command_bundle(ze_device_handle_t device,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(device));
  });
}

command_bundle create_command_bundle(ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(device, list_flags));
  });
}

command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(context, device));
  });
}

command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(context, device, list_flags));
  });
}

command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     uint32_t ordinal,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(
        create_command_bundle<M>(context, device, list_flags, ordinal));
  });
}

command_bundle create_command_bundle(ze_device_handle_t device,
                                     ze_command_queue_flags_t queue_flags,
                                     ze_command_queue_mode_t queue_mode,
                                     ze_command_queue_priority_t priority,
                                     ze_command_list_flags_t list_flags,
                                     uint32_t ordinal,
                                     command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(device, queue_flags, queue_mode,
                                               priority, list_flags, ordinal));
  });
}

command_bundle create_command_bundle(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t queue_flags, ze_command_queue_mode_t queue_mode,
    ze_command_queue_priority_t priority, ze_command_list_flags_t list_flags,
    uint32_t ordinal, uint32_t index, command_list_mode_t mode) {
  return dispatch_mode(mode, [&]<command_list_mode_t M>() {
    return to_runtime(create_command_bundle<M>(context, device, queue_flags,
                                               queue_mode, priority, list_flags,
                                               ordinal, index));
  });
}

void close_command_bundle(command_bundle &bundle) {
  dispatch_mode(bundle.mode, [&]<command_list_mode_t M>() {
    auto typed = to_typed<M>(bundle);
    close_command_bundle<M>(typed);
  });
}

void submit_command_bundle(command_bundle &bundle) {
  dispatch_mode(bundle.mode, [&]<command_list_mode_t M>() {
    auto typed = to_typed<M>(bundle);
    submit_command_bundle<M>(typed);
  });
}

void sync_command_bundle(command_bundle &bundle, uint64_t timeout) {
  dispatch_mode(bundle.mode, [&]<command_list_mode_t M>() {
    auto typed = to_typed<M>(bundle);
    sync_command_bundle<M>(typed, timeout);
  });
}

void execute_and_sync_command_bundle(command_bundle bundle, uint64_t timeout) {
  close_command_bundle(bundle);
  submit_command_bundle(bundle);
  sync_command_bundle(bundle, timeout);
}

void destroy_command_bundle(command_bundle bundle) {
  dispatch_mode(bundle.mode, [&]<command_list_mode_t M>() {
    destroy_command_bundle<M>(to_typed<M>(bundle));
  });
}

} // namespace level_zero_tests
