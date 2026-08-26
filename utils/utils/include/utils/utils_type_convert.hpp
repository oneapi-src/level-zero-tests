/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_UTILS_TYPE_CONVERT_HPP
#define level_zero_tests_UTILS_TYPE_CONVERT_HPP

#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace level_zero_tests {

template <typename T, typename U> inline constexpr T to_type(U val) {
  return static_cast<T>(val);
}

template <typename T> inline constexpr char to_char(T val) {
  return to_type<char>(val);
}
template <typename T> inline constexpr unsigned char to_uchar(T val) {
  return to_type<unsigned char>(val);
}

template <typename T> inline constexpr short to_short(T val) {
  return to_type<short>(val);
}
template <typename T> inline constexpr unsigned short to_ushort(T val) {
  return to_type<unsigned short>(val);
}

template <typename T> inline constexpr int to_int(T val) {
  return to_type<int>(val);
}
template <typename T> inline constexpr unsigned int to_uint(T val) {
  return to_type<unsigned int>(val);
}

template <typename T> inline constexpr long to_long(T val) {
  return to_type<long>(val);
}
template <typename T> inline constexpr unsigned long to_ulong(T val) {
  return to_type<unsigned long>(val);
}

template <typename T> inline constexpr long long to_llong(T val) {
  return to_type<long long>(val);
}
template <typename T> inline constexpr unsigned long long to_ullong(T val) {
  return to_type<unsigned long long>(val);
}

template <typename T> inline constexpr std::int8_t to_s8(T val) {
  return to_type<std::int8_t>(val);
}
template <typename T> inline constexpr std::uint8_t to_u8(T val) {
  return to_type<std::uint8_t>(val);
}

template <typename T> inline constexpr std::int16_t to_s16(T val) {
  return to_type<std::int16_t>(val);
}
template <typename T> inline constexpr std::uint16_t to_u16(T val) {
  return to_type<std::uint16_t>(val);
}

template <typename T> inline constexpr std::int32_t to_s32(T val) {
  return to_type<std::int32_t>(val);
}
template <> inline std::int32_t to_s32<const char *>(const char *str) {
  return to_type<std::int32_t>(std::strtol(str, nullptr, 10));
}
template <> inline std::int32_t to_s32<char *>(char *str) {
  return to_type<std::int32_t>(std::strtol(str, nullptr, 10));
}
template <typename T> inline constexpr std::uint32_t to_u32(T val) {
  return to_type<std::uint32_t>(val);
}
template <> inline std::uint32_t to_u32<const char *>(const char *str) {
  return to_type<std::uint32_t>(std::strtoul(str, nullptr, 10));
}
template <> inline std::uint32_t to_u32<char *>(char *str) {
  return to_type<std::uint32_t>(std::strtoul(str, nullptr, 10));
}

template <typename T> inline constexpr std::int64_t to_s64(T val) {
  return to_type<std::int64_t>(val);
}
template <typename T> inline constexpr std::uint64_t to_u64(T val) {
  return to_type<std::uint64_t>(val);
}
template <typename T> inline constexpr std::size_t to_size(T val) {
  return to_type<std::size_t>(val);
}

template <typename T> inline constexpr float to_f32(T val) {
  return to_type<float>(val);
}
template <typename T> inline constexpr double to_f64(T val) {
  return to_type<double>(val);
}

template <typename T, typename U> inline T *as_ptr(U *ptr) {
  return reinterpret_cast<T *>(ptr);
}

template <typename T, typename U> inline const T *as_const_ptr(const U *ptr) {
  return reinterpret_cast<const T *>(ptr);
}

} // namespace level_zero_tests

#endif // level_zero_tests_UTILS_TYPE_CONVERT_HPP
