/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _NUMA_SUPPORT_HPP_
#define _NUMA_SUPPORT_HPP_

#include <cstddef>
#include <string>
#include <vector>

//---------------------------------------------------------------------
// Host side NUMA placement, built on the kernel interfaces directly rather
// than on libnuma, so that the perf tests gain no new build dependency.
//
// Everything here is a no-op outside Linux, and every entry point reports
// failure instead of terminating: placement is an optimisation of the
// measurement, never a precondition for it.
//---------------------------------------------------------------------
namespace numa_support {

// Nothing requested: the allocation keeps whatever placement the kernel picks.
constexpr int no_node = -1;

// True when the running system exposes NUMA nodes through sysfs.
bool supported(void);

// Online node ids, empty when the system is not NUMA capable.
const std::vector<int> &online_nodes(void);

bool node_online(int node);

size_t page_size(void);

//---------------------------------------------------------------------
// Applies MPOL_BIND over [ptr, ptr + bytes), retrying without MPOL_MF_MOVE
// when the already resident pages cannot be migrated, so that at least the
// pages faulted in later land on the requested node.
//
// The range is rounded out to whole pages, because mbind() has page
// granularity. Pass a page aligned pointer, or the binding reaches into
// whatever shares the first page.
//---------------------------------------------------------------------
bool bind_memory(void *ptr, size_t bytes, int node, std::string &error);

//---------------------------------------------------------------------
// Node that actually backs the page holding ptr, or no_node when the kernel
// will not say. This is a single page sample, not a survey of the range.
//
// A driver mapping has no page the allocator knows about, so this reports
// no_node for one even after a binding was accepted, which is exactly the
// case worth warning about.
//---------------------------------------------------------------------
int memory_node(const void *ptr);

//---------------------------------------------------------------------
// Anonymous memory faulted in on a node, which is the placement mbind() can
// actually control. Memory a driver hands back from its own mapping is
// outside the page allocator, so a binding applied to it is accepted and
// then ignored; this is the allocation to import into Level Zero instead.
//
// Returns nullptr on failure, with the reason in error.
//---------------------------------------------------------------------
void *allocate_on_node(size_t bytes, int node, std::string &error);
void free_on_node(void *ptr, size_t bytes);

//---------------------------------------------------------------------
// Writes one byte per page so that the binding above is resolved before the
// measurement instead of during it.
//---------------------------------------------------------------------
void touch_pages(void *ptr, size_t bytes);

// CPUs belonging to a node, empty when the node is unknown.
std::vector<int> node_cpus(int node);

//---------------------------------------------------------------------
// Pins the calling thread to the CPUs of a node, restoring the previous
// affinity mask on destruction. Constructing with no_node, or on a system
// without NUMA, leaves the thread alone.
//---------------------------------------------------------------------
class ScopedCpuBind {
public:
  explicit ScopedCpuBind(int node);
  ~ScopedCpuBind();

  ScopedCpuBind(const ScopedCpuBind &) = delete;
  ScopedCpuBind &operator=(const ScopedCpuBind &) = delete;

  bool active(void) const { return active_; }
  const std::string &error(void) const { return error_; }

private:
  bool active_ = false;
  std::string error_;
  // Opaque storage for the mask that was in force before this object, kept
  // untyped so that the header does not drag <sched.h> into every caller.
  std::vector<unsigned char> saved_mask_;
};

} // namespace numa_support

#endif /* _NUMA_SUPPORT_HPP_ */
