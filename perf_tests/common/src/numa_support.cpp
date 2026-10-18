/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "numa_support.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

#ifdef __linux__
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

// mbind(), get_mempolicy() and move_pages() have no glibc wrapper, so they
// are reached through syscall(). A port without the numbers simply loses the
// feature.
#if defined(__linux__) && defined(__NR_mbind) &&                               \
    defined(__NR_get_mempolicy) && defined(__NR_move_pages)
#define LZT_NUMA_SYSCALLS 1
#endif

namespace numa_support {

namespace {

#ifdef __linux__
constexpr int mpol_bind = 2;
constexpr unsigned mpol_mf_move = 1u << 1;
constexpr int mpol_f_node = 1 << 0;
constexpr int mpol_f_addr = 1 << 1;

// Matches MAX_NUMNODES on a distribution kernel; a larger id is rejected
// here rather than handed to mbind() as a malformed mask.
constexpr int highest_supported_node = 1023;

constexpr size_t bits_per_word = 8 * sizeof(unsigned long);

std::string read_sysfs_line(const std::string &path) {
  std::ifstream file(path);
  std::string line;

  std::getline(file, line);
  return line;
}

//---------------------------------------------------------------------
// Parses the "0-3,8,12-15" form shared by the node and cpulist files. A
// malformed entry is skipped: sysfs would have to be broken to produce one,
// and losing a CPU is better than aborting a benchmark over it.
//---------------------------------------------------------------------
std::vector<int> parse_range_list(const std::string &text) {
  std::vector<int> values;
  std::stringstream stream(text);
  std::string token;

  while (std::getline(stream, token, ',')) {
    if (token.empty()) {
      continue;
    }

    const size_t dash = token.find('-');
    try {
      const int first = std::stoi(token.substr(0, dash));
      const int last = (dash == std::string::npos)
                           ? first
                           : std::stoi(token.substr(dash + 1));

      for (int value = first; value <= last; value++) {
        values.push_back(value);
      }
    } catch (const std::exception &) {
      continue;
    }
  }

  return values;
}

//---------------------------------------------------------------------
// sched_getaffinity() refuses a mask smaller than the kernel's cpumask and
// does not report the size it wants, so the mask is grown until the call is
// accepted. The size that worked is what sched_setaffinity() must be given.
//---------------------------------------------------------------------
bool read_affinity(size_t minimum_cpus, std::vector<unsigned char> &mask) {
  for (size_t cpus = std::max<size_t>(minimum_cpus, 1024); cpus <= 1 << 20;
       cpus *= 2) {
    const size_t bytes = CPU_ALLOC_SIZE(cpus);
    mask.assign(bytes, 0);

    if (sched_getaffinity(0, bytes,
                          reinterpret_cast<cpu_set_t *>(mask.data())) == 0) {
      return true;
    }

    if (errno != EINVAL) {
      return false;
    }
  }

  return false;
}
#endif

std::vector<int> discover_online_nodes(void) {
#ifdef __linux__
  return parse_range_list(read_sysfs_line("/sys/devices/system/node/online"));
#else
  return {};
#endif
}

} // namespace

const std::vector<int> &online_nodes(void) {
  static const std::vector<int> nodes = discover_online_nodes();
  return nodes;
}

bool supported(void) { return !online_nodes().empty(); }

bool node_online(int node) {
  const std::vector<int> &nodes = online_nodes();
  return std::find(nodes.begin(), nodes.end(), node) != nodes.end();
}

size_t page_size(void) {
#ifdef __linux__
  static const size_t size = []() {
    const long reported = sysconf(_SC_PAGESIZE);
    return (reported > 0) ? static_cast<size_t>(reported) : size_t{4096};
  }();
  return size;
#else
  return 4096;
#endif
}

bool bind_memory(void *ptr, size_t bytes, int node, std::string &error) {
  error.clear();

  if (node == no_node) {
    return true;
  }

#ifdef LZT_NUMA_SYSCALLS
  if (ptr == nullptr || bytes == 0) {
    error = "the allocation is empty";
    return false;
  }

  if (node < 0 || node > highest_supported_node) {
    error = "node " + std::to_string(node) + " is out of range";
    return false;
  }

  const uintptr_t mask_off = static_cast<uintptr_t>(page_size()) - 1;
  const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
  const uintptr_t first_page = address & ~mask_off;
  const uintptr_t past_last_page = (address + bytes + mask_off) & ~mask_off;

  const size_t words = static_cast<size_t>(node) / bits_per_word + 1;
  std::vector<unsigned long> nodemask(words, 0UL);
  nodemask[static_cast<size_t>(node) / bits_per_word] =
      1UL << (static_cast<size_t>(node) % bits_per_word);

  // Pages already resident are migrated when the kernel allows it. When they
  // are pinned it refuses, and the policy on its own still governs every
  // page faulted in afterwards, which is the common case here because the
  // binding is applied before the buffer is ever touched.
  for (const unsigned flags : {mpol_mf_move, 0u}) {
    const long status =
        syscall(__NR_mbind, reinterpret_cast<void *>(first_page),
                static_cast<unsigned long>(past_last_page - first_page),
                mpol_bind, nodemask.data(), words * bits_per_word, flags);

    if (status == 0) {
      error.clear();
      return true;
    }

    if (error.empty()) {
      error = std::strerror(errno);
    }
  }

  return false;
#else
  (void)ptr;
  (void)bytes;
  error = "NUMA placement is only implemented on Linux";
  return false;
#endif
}

int memory_node(const void *ptr) {
#ifdef LZT_NUMA_SYSCALLS
  if (ptr == nullptr) {
    return no_node;
  }

  // move_pages() answers for any mapping and says "not present" rather than
  // failing when the address has no page the allocator owns, which is how a
  // driver mapping is told apart from an unplaced one.
  void *pages[1] = {const_cast<void *>(ptr)};
  int status[1] = {no_node};
  if (syscall(__NR_move_pages, 0, 1UL, pages, nullptr, status, 0) == 0 &&
      status[0] >= 0) {
    return status[0];
  }

  int node = no_node;
  const long result =
      syscall(__NR_get_mempolicy, &node, nullptr, 0UL, const_cast<void *>(ptr),
              mpol_f_node | mpol_f_addr);

  return (result == 0) ? node : no_node;
#else
  (void)ptr;
  return no_node;
#endif
}

void *allocate_on_node(size_t bytes, int node, std::string &error) {
  error.clear();

#ifdef LZT_NUMA_SYSCALLS
  if (bytes == 0) {
    error = "the allocation is empty";
    return nullptr;
  }

  void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED) {
    error = std::strerror(errno);
    return nullptr;
  }

  if (!bind_memory(ptr, bytes, node, error)) {
    munmap(ptr, bytes);
    return nullptr;
  }

  // The binding decides where a page lands as it is faulted in, so the whole
  // range is faulted here, while the node is still the one that was asked
  // for and before anything is timed.
  touch_pages(ptr, bytes);

  const int placed = memory_node(ptr);
  if (placed != node) {
    error = "the pages landed on node " + std::to_string(placed) +
            " instead of node " + std::to_string(node);
    munmap(ptr, bytes);
    return nullptr;
  }

  return ptr;
#else
  (void)bytes;
  (void)node;
  error = "NUMA placement is only implemented on Linux";
  return nullptr;
#endif
}

void free_on_node(void *ptr, size_t bytes) {
#ifdef LZT_NUMA_SYSCALLS
  if (ptr != nullptr && bytes != 0) {
    munmap(ptr, bytes);
  }
#else
  (void)ptr;
  (void)bytes;
#endif
}

void touch_pages(void *ptr, size_t bytes) {
  if (ptr == nullptr || bytes == 0) {
    return;
  }

  volatile unsigned char *data = static_cast<volatile unsigned char *>(ptr);
  const size_t stride = page_size();

  for (size_t offset = 0; offset < bytes; offset += stride) {
    data[offset] = 0;
  }
  data[bytes - 1] = 0;
}

std::vector<int> node_cpus(int node) {
#ifdef __linux__
  if (node < 0) {
    return {};
  }

  return parse_range_list(read_sysfs_line("/sys/devices/system/node/node" +
                                          std::to_string(node) + "/cpulist"));
#else
  (void)node;
  return {};
#endif
}

ScopedCpuBind::ScopedCpuBind(int node) {
  if (node == no_node) {
    return;
  }

#ifdef __linux__
  const std::vector<int> cpus = node_cpus(node);
  if (cpus.empty()) {
    error_ = "node " + std::to_string(node) + " lists no CPUs";
    return;
  }

  const int highest = *std::max_element(cpus.begin(), cpus.end());
  if (highest < 0) {
    error_ = "node " + std::to_string(node) + " lists no usable CPU";
    return;
  }

  std::vector<unsigned char> previous;
  if (!read_affinity(static_cast<size_t>(highest) + 1, previous)) {
    error_ = std::strerror(errno);
    return;
  }

  std::vector<unsigned char> wanted(previous.size(), 0);
  cpu_set_t *set = reinterpret_cast<cpu_set_t *>(wanted.data());
  CPU_ZERO_S(wanted.size(), set);
  for (const int cpu : cpus) {
    if (cpu >= 0 && static_cast<size_t>(cpu) < wanted.size() * 8) {
      CPU_SET_S(static_cast<size_t>(cpu), wanted.size(), set);
    }
  }

  if (sched_setaffinity(0, wanted.size(), set) != 0) {
    error_ = std::strerror(errno);
    return;
  }

  saved_mask_ = std::move(previous);
  active_ = true;
#else
  error_ = "CPU affinity is only implemented on Linux";
#endif
}

ScopedCpuBind::~ScopedCpuBind() {
#ifdef __linux__
  if (!active_ || saved_mask_.empty()) {
    return;
  }

  sched_setaffinity(0, saved_mask_.size(),
                    reinterpret_cast<cpu_set_t *>(saved_mask_.data()));
#endif
}

} // namespace numa_support
