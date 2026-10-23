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
// placement.
#if defined(__linux__) && defined(__NR_mbind) &&                               \
    defined(__NR_get_mempolicy) && defined(__NR_move_pages)
#define LZT_NUMA_SYSCALLS 1
#endif

namespace numa_support {

namespace {

#ifdef LZT_NUMA_SYSCALLS
constexpr int mpol_bind = 2;
constexpr int mpol_f_node = 1 << 0;
constexpr int mpol_f_addr = 1 << 1;

// Matches MAX_NUMNODES on a distribution kernel; a larger id is rejected
// here rather than handed to mbind() as a malformed mask.
constexpr int highest_supported_node = 1023;

constexpr size_t bits_per_word = 8 * sizeof(unsigned long);

constexpr size_t huge_page_size = size_t{2} << 20;

uintptr_t align_up(uintptr_t value, size_t alignment) {
  return (value + alignment - 1) & ~static_cast<uintptr_t>(alignment - 1);
}

// The range has not been touched yet, so the policy decides where every page
// lands as it is faulted in.
bool bind_memory(void *ptr, size_t bytes, int node, std::string &error) {
  if (node < 0 || node > highest_supported_node) {
    error = "node " + std::to_string(node) + " is out of range";
    return false;
  }

  const size_t words = static_cast<size_t>(node) / bits_per_word + 1;
  std::vector<unsigned long> nodemask(words, 0UL);
  nodemask[static_cast<size_t>(node) / bits_per_word] =
      1UL << (static_cast<size_t>(node) % bits_per_word);

  // The kernel decrements maxnode before reading the mask (get_nodes() in
  // mm/mempolicy.c), so one more than the mask width keeps its top bit.
  if (syscall(__NR_mbind, ptr, static_cast<unsigned long>(bytes), mpol_bind,
              nodemask.data(), words * bits_per_word + 1, 0u) != 0) {
    error = std::string("mbind failed: ") + std::strerror(errno);
    return false;
  }

  return true;
}

// Node that backs the page holding ptr, if the kernel will say.
std::optional<int> memory_node(const void *ptr) {
  void *pages[1] = {const_cast<void *>(ptr)};
  int status[1] = {-1};
  if (syscall(__NR_move_pages, 0, 1UL, pages, nullptr, status, 0) == 0 &&
      status[0] >= 0) {
    return status[0];
  }

  int node = -1;
  if (syscall(__NR_get_mempolicy, &node, nullptr, 0UL, const_cast<void *>(ptr),
              mpol_f_node | mpol_f_addr) == 0 &&
      node >= 0) {
    return node;
  }
  return std::nullopt;
}

void touch_pages(void *ptr, size_t bytes) {
  volatile unsigned char *data = static_cast<volatile unsigned char *>(ptr);
  const size_t stride = page_size();

  for (size_t offset = 0; offset < bytes; offset += stride) {
    data[offset] = 0;
  }
}
#endif

#ifdef __linux__
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

std::vector<int> discover_online_nodes() {
#ifdef __linux__
  return parse_range_list(read_sysfs_line("/sys/devices/system/node/online"));
#else
  return {};
#endif
}

} // namespace

const std::vector<int> &online_nodes() {
  static const std::vector<int> nodes = discover_online_nodes();
  return nodes;
}

bool node_online(int node) {
  const std::vector<int> &nodes = online_nodes();
  return std::find(nodes.begin(), nodes.end(), node) != nodes.end();
}

size_t page_size() {
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

void *allocate_on_node([[maybe_unused]] size_t bytes, [[maybe_unused]] int node,
                       size_t &mapped_bytes, std::string &error) {
  error.clear();
  mapped_bytes = 0;

#ifdef LZT_NUMA_SYSCALLS
  if (bytes == 0) {
    error = "the allocation is empty";
    return nullptr;
  }

  const size_t alignment =
      (bytes >= huge_page_size) ? huge_page_size : page_size();
  const size_t length = align_up(bytes, alignment);

  // mmap() only guarantees page alignment, so a larger alignment is cut out
  // of a reservation that is one alignment longer.
  const size_t reserved =
      length + ((alignment > page_size()) ? alignment : size_t{0});
  void *raw = mmap(nullptr, reserved, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (raw == MAP_FAILED) {
    error = std::string("mmap failed: ") + std::strerror(errno);
    return nullptr;
  }

  const uintptr_t raw_begin = reinterpret_cast<uintptr_t>(raw);
  const uintptr_t raw_end = raw_begin + reserved;
  const uintptr_t begin = align_up(raw_begin, alignment);
  const uintptr_t end = begin + length;
  if (begin > raw_begin) {
    munmap(raw, begin - raw_begin);
  }
  if (raw_end > end) {
    munmap(reinterpret_cast<void *>(end), raw_end - end);
  }
  void *ptr = reinterpret_cast<void *>(begin);

#ifdef MADV_HUGEPAGE
  // Only advice: with transparent huge pages disabled the range is still
  // usable, merely backed by base pages.
  if (alignment == huge_page_size) {
    madvise(ptr, length, MADV_HUGEPAGE);
  }
#endif

  if (!bind_memory(ptr, length, node, error)) {
    munmap(ptr, length);
    return nullptr;
  }

  touch_pages(ptr, length);

  const void *last_page = static_cast<unsigned char *>(ptr) + length - 1;
  for (const void *page : {static_cast<const void *>(ptr), last_page}) {
    const std::optional<int> placed = memory_node(page);
    if (placed != node) {
      error = "the pages landed on node " +
              (placed ? std::to_string(*placed) : std::string("unknown")) +
              " instead of node " + std::to_string(node);
      munmap(ptr, length);
      return nullptr;
    }
  }

  mapped_bytes = length;
  return ptr;
#else
  error = "NUMA placement is only implemented on Linux";
  return nullptr;
#endif
}

void free_on_node([[maybe_unused]] void *ptr,
                  [[maybe_unused]] size_t mapped_bytes) {
#ifdef LZT_NUMA_SYSCALLS
  if (ptr != nullptr && mapped_bytes != 0) {
    munmap(ptr, mapped_bytes);
  }
#endif
}

std::vector<int> node_cpus([[maybe_unused]] int node) {
#ifdef __linux__
  if (node < 0) {
    return {};
  }

  return parse_range_list(read_sysfs_line("/sys/devices/system/node/node" +
                                          std::to_string(node) + "/cpulist"));
#else
  return {};
#endif
}

ScopedCpuBind::ScopedCpuBind(std::optional<int> node) {
  if (!node) {
    return;
  }

#ifdef __linux__
  const std::vector<int> cpus = node_cpus(*node);
  if (cpus.empty()) {
    error_ = "node " + std::to_string(*node) + " lists no CPUs";
    return;
  }

  const int highest = *std::max_element(cpus.begin(), cpus.end());
  if (highest < 0) {
    error_ = "node " + std::to_string(*node) + " lists no usable CPU";
    return;
  }

  std::vector<unsigned char> previous;
  if (!read_affinity(static_cast<size_t>(highest) + 1, previous)) {
    error_ = std::strerror(errno);
    return;
  }

  const cpu_set_t *allowed = reinterpret_cast<cpu_set_t *>(previous.data());
  std::vector<unsigned char> wanted(previous.size(), 0);
  cpu_set_t *set = reinterpret_cast<cpu_set_t *>(wanted.data());
  CPU_ZERO_S(wanted.size(), set);
  for (const int cpu : cpus) {
    if (cpu >= 0 && static_cast<size_t>(cpu) < wanted.size() * 8 &&
        CPU_ISSET_S(static_cast<size_t>(cpu), previous.size(), allowed)) {
      CPU_SET_S(static_cast<size_t>(cpu), wanted.size(), set);
    }
  }

  if (CPU_COUNT_S(wanted.size(), set) == 0) {
    error_ = "the current CPU affinity allows no CPU of node " +
             std::to_string(*node) + ", left unchanged";
    return;
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
