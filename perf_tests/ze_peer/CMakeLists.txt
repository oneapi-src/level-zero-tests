# Copyright (C) 2019-2026 Intel Corporation
# SPDX-License-Identifier: MIT

if(UNIX)
    set(OS_SPECIFIC_LIBS pthread rt)
else()
    set(OS_SPECIFIC_LIBS "")
endif()

add_lzt_test(
  NAME ze_peer
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/ze_peer.cpp
    src/ze_peer_ipc.cpp
    src/ze_peer_unidirectional.cpp
    src/ze_peer_bidirectional.cpp
    src/ze_peer_parallel_multiple_targets.cpp
    src/ze_peer_parallel_pair_targets.cpp
    src/ze_peer_parallel_single_target.cpp
    src/ze_peer_common.cpp
  LINK_LIBRARIES ${OS_SPECIFIC_LIBS}
  KERNELS ze_peer_benchmarks
)

# These run without a GPU: options are parsed before any device is opened.
if(TARGET ze_peer)
  add_test(NAME ze_peer_help_lists_remote_wait COMMAND ze_peer --help)
  set_tests_properties(ze_peer_help_lists_remote_wait
    PROPERTIES PASS_REGULAR_EXPRESSION "--remote_wait")

  function(add_ze_peer_remote_wait_reject_test name)
    add_test(NAME ze_peer_remote_wait_rejects_${name}
      COMMAND ze_peer ${ARGN} --remote_wait)
    set_tests_properties(ze_peer_remote_wait_rejects_${name}
      PROPERTIES PASS_REGULAR_EXPRESSION "--remote_wait is not supported"
                 TIMEOUT 30)
  endfunction()

  add_ze_peer_remote_wait_reject_test(bidirectional -b)
  add_ze_peer_remote_wait_reject_test(ipc --ipc)
  add_ze_peer_remote_wait_reject_test(parallel_single_target
    --parallel_single_target)
  add_ze_peer_remote_wait_reject_test(parallel_multiple_targets
    --parallel_multiple_targets)
  add_ze_peer_remote_wait_reject_test(parallel_pair_targets
    --parallel_pair_targets 0:1)
endif()
