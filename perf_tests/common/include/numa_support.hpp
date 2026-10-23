/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

//---------------------------------------------------------------------
// Host side NUMA topology and thread placement, read from sysfs and applied
// with sched_setaffinity() rather than through libnuma, so that the perf
// tests gain no new build dependency.
//
// Everything here is a no-op outside Linux, and every entry point reports
// failure instead of terminating: placement is an optimisation of the
// measurement, never a precondition for it.
//---------------------------------------------------------------------
namespace numa_support {

// Online node ids, empty when the system is not NUMA capable.
const std::vector<int> &online_nodes();

bool node_online(int node);

size_t page_size();

//---------------------------------------------------------------------
// Anonymous memory bound to a node with mbind() and faulted in there before
// it is returned, so that the placement is the application's. Ranges of 2 MB
// and above are 2 MB aligned and advised for transparent huge pages, so that
// they are not backed by smaller pages than a driver allocation of that size.
// The node of the first and of the last page is checked.
//
// mapped_bytes receives the length actually mapped, a whole number of pages,
// which is what free_on_node() takes back. Returns nullptr on failure, with
// the reason in error.
//---------------------------------------------------------------------
void *allocate_on_node(size_t bytes, int node, size_t &mapped_bytes,
                       std::string &error);
void free_on_node(void *ptr, size_t mapped_bytes);

// CPUs belonging to a node, empty when the node is unknown.
std::vector<int> node_cpus(int node);

//---------------------------------------------------------------------
// Pins the calling thread to the CPUs of a node that its current affinity
// mask allows, so that a mask set with taskset or numactl is narrowed, never
// widened. The previous mask is restored on destruction. No node, a system
// without NUMA, or a mask that allows no CPU of the node leaves the thread
// alone; error() then says why.
//---------------------------------------------------------------------
class ScopedCpuBind {
public:
  explicit ScopedCpuBind(std::optional<int> node);
  ~ScopedCpuBind();

  ScopedCpuBind(const ScopedCpuBind &) = delete;
  ScopedCpuBind &operator=(const ScopedCpuBind &) = delete;

  bool active() const { return active_; }
  const std::string &error() const { return error_; }

private:
  bool active_ = false;
  std::string error_;
  // Opaque storage for the mask that was in force before this object, kept
  // untyped so that the header does not drag <sched.h> into every caller.
  std::vector<unsigned char> saved_mask_;
};

} // namespace numa_support
