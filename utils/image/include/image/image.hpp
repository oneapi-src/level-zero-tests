/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_IMAGE_HPP
#define level_zero_tests_IMAGE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace level_zero_tests {
template <typename T> class Image {
public:
  virtual ~Image() = default;
  virtual bool read(const std::string &image_path) = 0;
  virtual bool write(const std::string &image_path) = 0;
  virtual bool write(const std::string &image_path, const T *data) = 0;
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual uint16_t number_of_channels() const = 0;
  virtual uint16_t bits_per_channel() const = 0;
  virtual uint16_t bits_per_pixel() const = 0;
  virtual size_t size() const = 0;
  virtual size_t size_in_bytes() const = 0;
  virtual T get_pixel(const uint32_t x, const uint32_t y) const = 0;
  virtual void set_pixel(const uint32_t x, const uint32_t y, const T data) = 0;
  virtual std::vector<T> get_pixels() const = 0;
  virtual void copy_raw_data(const T *data) = 0;
  virtual T *raw_data() = 0;
  virtual const T *raw_data() const = 0;
};

template <typename T> class ImagePNG : public Image<T> {
public:
  ImagePNG();
  ImagePNG(const std::string &image_path);
  ImagePNG(const uint32_t width, const uint32_t height);
  ImagePNG(const uint32_t width, const uint32_t height,
           const std::vector<T> &data);
  bool read(const std::string &image_path) override;
  bool write(const std::string &image_path) override;
  bool write(const std::string &image_path, const T *data) override;
  uint32_t width() const override;
  uint32_t height() const override;
  uint16_t number_of_channels() const override;
  uint16_t bits_per_channel() const override;
  uint16_t bits_per_pixel() const override;
  size_t size() const override;
  size_t size_in_bytes() const override;
  T get_pixel(const uint32_t x, const uint32_t y) const override;
  void set_pixel(const uint32_t x, const uint32_t y, const T data) override;
  std::vector<T> get_pixels() const override;
  void copy_raw_data(const T *data) override;
  T *raw_data() override;
  const T *raw_data() const override;

  bool operator==(const ImagePNG &rhs) const;
  void dump_image() const;

private:
  std::vector<T> pixels_;
  uint32_t width_;
  uint32_t height_;
};

typedef ImagePNG<uint32_t> ImagePNG32Bit;

template <typename T> class ImageBMP : public Image<T> {
public:
  ImageBMP();
  ImageBMP(const std::string &image_path);
  ImageBMP(const uint32_t width, const uint32_t height);
  ImageBMP(const uint32_t width, const uint32_t height,
           const std::vector<T> &data);
  bool read(const std::string &image_path) override;
  bool write(const std::string &image_path) override;
  bool write(const std::string &image_path, const T *data) override;
  uint32_t width() const override;
  uint32_t height() const override;
  uint16_t number_of_channels() const override;
  uint16_t bits_per_channel() const override;
  uint16_t bits_per_pixel() const override;
  size_t size() const override;
  size_t size_in_bytes() const override;
  T get_pixel(const uint32_t x, const uint32_t y) const override;
  void set_pixel(const uint32_t x, const uint32_t y, const T data) override;
  std::vector<T> get_pixels() const override;
  void copy_raw_data(const T *data) override;
  T *raw_data() override;
  const T *raw_data() const override;

  bool operator==(const ImageBMP &rhs) const;

private:
  std::vector<T> pixels_;
  uint32_t width_;
  uint32_t height_;
};

typedef ImageBMP<uint8_t> ImageBMP8Bit;
typedef ImageBMP<uint32_t> ImageBMP32Bit;

template <typename T> size_t size_in_bytes(const Image<T> &i) {
  return i.size_in_bytes();
}

} // namespace level_zero_tests

#endif
