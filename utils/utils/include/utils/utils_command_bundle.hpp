/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_COMMAND_BUNDLE_HPP
#define level_zero_tests_UTILS_COMMAND_BUNDLE_HPP

#include <cstdint>
#include <type_traits>
#include <variant>

#include <level_zero/ze_api.h>

#include "test_harness/test_harness_cmdlist.hpp"
#include "test_harness/test_harness_cmdqueue.hpp"

namespace level_zero_tests {

ze_context_handle_t get_default_context();

enum class command_list_mode_t {
  regular = 0,
  immediate,
  immediate_append_regular
};

// https://en.cppreference.com/cpp/language/attributes/no_unique_address
#if defined(_MSC_VER) && !defined(__clang__)
#define LZT_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define LZT_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

// ---------------------------------------------------------------------------
// Compile-time-mode command bundle.
//
// Prefer over the runtime-mode command_bundle.
// Use the runtime-mode command_bundle when the
// mode is only known at runtime (e.g. value-parameterized tests).
// ---------------------------------------------------------------------------

template <command_list_mode_t Mode> struct command_bundle_t {
  static constexpr command_list_mode_t mode = Mode;

  static constexpr bool has_queue = (Mode == command_list_mode_t::regular);

  static constexpr bool has_append_list =
      (Mode == command_list_mode_t::immediate_append_regular);

  LZT_NO_UNIQUE_ADDRESS
  std::conditional_t<has_queue, ze_command_queue_handle_t, std::monostate>
      queue{};
  ze_command_list_handle_t list = nullptr;

  LZT_NO_UNIQUE_ADDRESS std::conditional_t<
      has_append_list, ze_command_list_handle_t, std::monostate>
      append_list{};

  // Returns the list that callers should append commands to. For
  // immediate_append_regular this is the recorded (append) list; for the
  // other modes it is the primary list.
  ze_command_list_handle_t record_list() const {
    if constexpr (has_append_list) {
      return append_list;
    } else {
      return list;
    }
  }

  // Returns the list used for submission / synchronization. Always the
  // primary (immediate or regular) list.
  ze_command_list_handle_t submit_list() const { return list; }

  explicit operator ze_command_list_handle_t() const { return record_list(); }
};

namespace detail {

constexpr bool mode_is_immediate(command_list_mode_t mode) {
  return mode != command_list_mode_t::regular;
}

template <command_list_mode_t> struct always_false : std::false_type {};

template <command_list_mode_t Mode>
command_bundle_t<Mode> make_bundle(zeCommandBundle legacy,
                                   ze_command_list_handle_t append_list) {
  command_bundle_t<Mode> b;
  if constexpr (command_bundle_t<Mode>::has_queue) {
    b.queue = legacy.queue;
  } else {
    (void)legacy.queue;
  }
  b.list = legacy.list;
  if constexpr (command_bundle_t<Mode>::has_append_list) {
    b.append_list = append_list;
  } else {
    (void)append_list;
  }
  return b;
}

} // namespace detail

template <command_list_mode_t Mode, class... Args>
command_bundle_t<Mode> create_command_bundle(Args... args) {
  auto legacy = create_command_bundle(args..., detail::mode_is_immediate(Mode));
  ze_command_list_handle_t append_list = nullptr;
  if constexpr (Mode == command_list_mode_t::immediate_append_regular) {
    append_list = create_command_list(args...);
  }
  return detail::make_bundle<Mode>(legacy, append_list);
}

template <command_list_mode_t Mode>
command_bundle_t<Mode> create_command_bundle(
    ze_device_handle_t device, ze_command_queue_flags_t queue_flags,
    ze_command_queue_mode_t queue_mode, ze_command_queue_priority_t priority,
    ze_command_list_flags_t list_flags, uint32_t ordinal) {
  auto legacy = create_command_bundle(device, queue_flags, queue_mode, priority,
                                      list_flags, ordinal,
                                      detail::mode_is_immediate(Mode));
  ze_command_list_handle_t append_list = nullptr;
  if constexpr (Mode == command_list_mode_t::immediate_append_regular) {
    append_list =
        create_command_list(get_default_context(), device, list_flags, ordinal);
  }
  return detail::make_bundle<Mode>(legacy, append_list);
}

template <command_list_mode_t Mode>
command_bundle_t<Mode> create_command_bundle(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t queue_flags, ze_command_queue_mode_t queue_mode,
    ze_command_queue_priority_t priority, ze_command_list_flags_t list_flags,
    uint32_t ordinal, uint32_t index) {
  auto legacy = create_command_bundle(context, device, queue_flags, queue_mode,
                                      priority, list_flags, ordinal, index,
                                      detail::mode_is_immediate(Mode));
  ze_command_list_handle_t append_list = nullptr;
  if constexpr (Mode == command_list_mode_t::immediate_append_regular) {
    append_list = create_command_list(context, device, list_flags, ordinal);
  }
  return detail::make_bundle<Mode>(legacy, append_list);
}

template <command_list_mode_t Mode>
void close_command_bundle([[maybe_unused]] command_bundle_t<Mode> &bundle) {
  if constexpr (Mode == command_list_mode_t::regular) {
    close_command_list(bundle.list);
  } else if constexpr (Mode == command_list_mode_t::immediate) {
    // no-op
  } else if constexpr (Mode == command_list_mode_t::immediate_append_regular) {
    close_command_list(bundle.append_list);
  } else {
    static_assert(detail::always_false<Mode>::value,
                  "Unsupported command_list_mode_t in close_command_bundle");
  }
}

template <command_list_mode_t Mode>
void submit_command_bundle([[maybe_unused]] command_bundle_t<Mode> &bundle) {
  if constexpr (Mode == command_list_mode_t::regular) {
    execute_command_lists(bundle.queue, 1, &bundle.list, nullptr);
  } else if constexpr (Mode == command_list_mode_t::immediate) {
    // no-op
  } else if constexpr (Mode == command_list_mode_t::immediate_append_regular) {
    ze_command_list_handle_t append = bundle.append_list;
    append_command_lists_immediate_exp(bundle.list, 1, &append);
  } else {
    static_assert(detail::always_false<Mode>::value,
                  "Unsupported command_list_mode_t in submit_command_bundle");
  }
}

template <command_list_mode_t Mode>
void sync_command_bundle(command_bundle_t<Mode> &bundle, uint64_t timeout) {
  if constexpr (Mode == command_list_mode_t::regular) {
    synchronize(bundle.queue, timeout);
  } else if constexpr (Mode == command_list_mode_t::immediate ||
                       Mode == command_list_mode_t::immediate_append_regular) {
    synchronize_command_list_host(bundle.list, timeout);
  } else {
    static_assert(detail::always_false<Mode>::value,
                  "Unsupported command_list_mode_t in sync_command_bundle");
  }
}

template <command_list_mode_t Mode>
void execute_and_sync_command_bundle(command_bundle_t<Mode> bundle,
                                     uint64_t timeout) {
  close_command_bundle<Mode>(bundle);
  submit_command_bundle<Mode>(bundle);
  sync_command_bundle<Mode>(bundle, timeout);
}

template <command_list_mode_t Mode>
void destroy_command_bundle(command_bundle_t<Mode> bundle) {
  if constexpr (command_bundle_t<Mode>::has_append_list) {
    if (bundle.append_list != nullptr) {
      destroy_command_list(bundle.append_list);
    }
  }
  destroy_command_list(bundle.list);
  if constexpr (command_bundle_t<Mode>::has_queue) {
    if (bundle.queue != nullptr) {
      destroy_command_queue(bundle.queue);
    }
  }
}

struct command_bundle {
  ze_command_queue_handle_t queue = nullptr;
  ze_command_list_handle_t list = nullptr;
  ze_command_list_handle_t append_list = nullptr;
  command_list_mode_t mode = command_list_mode_t::regular;

  ze_command_list_handle_t record_list() const {
    return append_list != nullptr ? append_list : list;
  }

  ze_command_list_handle_t submit_list() const { return list; }

  explicit operator ze_command_list_handle_t() const { return record_list(); }
};

command_bundle create_command_bundle(command_list_mode_t mode);
command_bundle create_command_bundle(ze_device_handle_t device,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(ze_context_handle_t context,
                                     ze_device_handle_t device,
                                     ze_command_list_flags_t list_flags,
                                     uint32_t ordinal,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(ze_device_handle_t device,
                                     ze_command_queue_flags_t queue_flags,
                                     ze_command_queue_mode_t queue_mode,
                                     ze_command_queue_priority_t priority,
                                     ze_command_list_flags_t list_flags,
                                     uint32_t ordinal,
                                     command_list_mode_t mode);
command_bundle create_command_bundle(
    ze_context_handle_t context, ze_device_handle_t device,
    ze_command_queue_flags_t queue_flags, ze_command_queue_mode_t queue_mode,
    ze_command_queue_priority_t priority, ze_command_list_flags_t list_flags,
    uint32_t ordinal, uint32_t index, command_list_mode_t mode);

void execute_and_sync_command_bundle(command_bundle bundle, uint64_t timeout);
void close_command_bundle(command_bundle &bundle);
void submit_command_bundle(command_bundle &bundle);
void sync_command_bundle(command_bundle &bundle, uint64_t timeout);
void destroy_command_bundle(command_bundle bundle);

} // namespace level_zero_tests

#endif // level_zero_tests_UTILS_COMMAND_BUNDLE_HPP
