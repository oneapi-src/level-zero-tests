/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "image/image.hpp"
#include "logging/logging.hpp"

#include <png.h>

#include <algorithm>
#include <csetjmp>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "bmp.hpp"

namespace level_zero_tests {

template <typename T> inline constexpr uint32_t to_u32(T val) {
  return static_cast<uint32_t>(val);
}

namespace {

constexpr size_t png_rgba_channels = 4;

[[noreturn]] void on_png_error(png_structp png, png_const_charp message) {
  *static_cast<std::string *>(png_get_error_ptr(png)) = message;
  png_longjmp(png, 1);
}

void on_png_warning(png_structp, png_const_charp message) {
  LOG_WARNING << "libpng: " << message;
}

void on_png_read(png_structp png, png_bytep data, png_size_t length) {
  auto *stream = static_cast<std::istream *>(png_get_io_ptr(png));
  if (!stream->read(reinterpret_cast<char *>(data),
                    static_cast<std::streamsize>(length))) {
    png_error(png, "unexpected end of file");
  }
}

class png_reader {
public:
  explicit png_reader(std::string &error)
      : png_(png_create_read_struct(PNG_LIBPNG_VER_STRING, &error, on_png_error,
                                    on_png_warning)),
        info_(png_ ? png_create_info_struct(png_) : nullptr) {
    if (info_ == nullptr) {
      png_destroy_read_struct(&png_, nullptr, nullptr);
      throw std::runtime_error("cannot create libpng read structures");
    }
  }
  ~png_reader() { png_destroy_read_struct(&png_, &info_, nullptr); }
  png_reader(const png_reader &) = delete;
  png_reader &operator=(const png_reader &) = delete;

  png_structp png() const { return png_; }
  png_infop info() const { return info_; }

private:
  png_structp png_;
  png_infop info_;
};

bool decode_rgba8(png_structp png, png_infop info, std::vector<uint8_t> &rgba,
                  uint32_t &width, uint32_t &height) {
  if (setjmp(png_jmpbuf(png))) {
    return false;
  }

  png_read_info(png, info);

  const png_byte color_type = png_get_color_type(png, info);
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png, info) < 8) {
    png_set_expand_gray_1_2_4_to_8(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
  }
  png_set_scale_16(png);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png);
  }
  png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  const int passes = png_set_interlace_handling(png);
  png_read_update_info(png, info);

  width = png_get_image_width(png, info);
  height = png_get_image_height(png, info);
  const size_t stride = static_cast<size_t>(width) * png_rgba_channels;
  if (png_get_rowbytes(png, info) != stride) {
    png_error(png, "unexpected row size after RGBA8 conversion");
  }
  rgba.assign(stride * height, 0);
  for (int pass = 0; pass < passes; ++pass) {
    for (uint32_t y = 0; y < height; ++y) {
      png_read_row(png, rgba.data() + y * stride, nullptr);
    }
  }
  png_read_end(png, nullptr);
  return true;
}

std::vector<uint8_t> read_png_rgba8(const std::string &path, uint32_t &width,
                                    uint32_t &height) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("cannot open file for reading");
  }
  std::string error;
  png_reader reader(error);
  png_set_read_fn(reader.png(), static_cast<std::istream *>(&file),
                  on_png_read);
  std::vector<uint8_t> rgba;
  if (!decode_rgba8(reader.png(), reader.info(), rgba, width, height)) {
    throw std::runtime_error(error);
  }
  return rgba;
}

} // namespace

template <typename T> ImagePNG<T>::ImagePNG() : width_(0U), height_(0U) {}

template <typename T> ImagePNG<T>::ImagePNG(const std::string &image_path) {
  read(image_path);
}

template <typename T>
ImagePNG<T>::ImagePNG(const uint32_t width, const uint32_t height)
    : width_(width), height_(height) {
  pixels_.resize(size());
}

template <typename T>
ImagePNG<T>::ImagePNG(const uint32_t width, const uint32_t height,
                      const std::vector<T> &data)
    : pixels_(data), width_(width), height_(height) {}

template <> bool ImagePNG<uint32_t>::read(const std::string &image_path) {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<uint8_t> rgba;
  try {
    rgba = read_png_rgba8(image_path, width, height);
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to read PNG image " + image_path + ": " +
                             e.what());
  }
  pixels_.clear();
  pixels_.reserve(rgba.size() / png_rgba_channels);
  for (size_t idx = 0; idx < rgba.size(); idx += png_rgba_channels) {
    pixels_.push_back((to_u32(rgba[idx]) << 24) |
                      (to_u32(rgba[idx + 1]) << 16) |
                      (to_u32(rgba[idx + 2]) << 8) | to_u32(rgba[idx + 3]));
  }
  width_ = width;
  height_ = height;
  return false;
}

template <typename T> uint32_t ImagePNG<T>::width() const { return width_; }

template <typename T> uint32_t ImagePNG<T>::height() const { return height_; }

template <typename T> uint16_t ImagePNG<T>::number_of_channels() const {
  return bits_per_pixel() / bits_per_channel();
}

template <typename T> uint16_t ImagePNG<T>::bits_per_channel() const {
  return 8;
}

template <typename T> uint16_t ImagePNG<T>::bits_per_pixel() const {
  return std::numeric_limits<T>::digits;
}

template <typename T> size_t ImagePNG<T>::size() const {
  return static_cast<size_t>(width()) * static_cast<size_t>(height());
}

template <typename T> size_t ImagePNG<T>::size_in_bytes() const {
  return pixels_.size() * sizeof(T);
}

template <typename T>
T ImagePNG<T>::get_pixel(const uint32_t x, const uint32_t y) const {
  return pixels_[y * width() + x];
}

template <typename T>
void ImagePNG<T>::set_pixel(const uint32_t x, const uint32_t y, const T data) {
  pixels_[y * width() + x] = data;
}

template <typename T> std::vector<T> ImagePNG<T>::get_pixels() const {
  return pixels_;
}

template <typename T> void ImagePNG<T>::copy_raw_data(const T *data) {
  std::copy(data, data + size(), std::begin(pixels_));
}

template <typename T> T *ImagePNG<T>::raw_data() { return pixels_.data(); }

template <typename T> const T *ImagePNG<T>::raw_data() const {
  return pixels_.data();
}

template <typename T>
bool ImagePNG<T>::operator==(const ImagePNG<T> &rhs) const {
  return pixels_ == rhs.pixels_;
}

template <typename T> void ImagePNG<T>::dump_image() const {
  for (uint32_t y = 0; y < height(); y++) {
    for (uint32_t x = 0; x < width(); x++) {
      T pixel = get_pixel(x, y);
      LOG_DEBUG << " x: " << x << " y: " << y << " pixel: 0x" << std::hex
                << pixel;
    }
  }
}

template class ImagePNG<uint32_t>;

template <typename T> ImageBMP<T>::ImageBMP() : width_(0U), height_(0U) {}

template <typename T> ImageBMP<T>::ImageBMP(const std::string &image_path) {
  read(image_path);
}

template <typename T>
ImageBMP<T>::ImageBMP(const uint32_t width, const uint32_t height)
    : width_(width), height_(height) {
  pixels_.resize(size());
}

template <typename T>
ImageBMP<T>::ImageBMP(const uint32_t width, const uint32_t height,
                      const std::vector<T> &data)
    : pixels_(data), width_(width), height_(height) {}

template <typename T> bool ImageBMP<T>::read(const std::string &image_path) {
  std::unique_ptr<uint8_t[]> data = nullptr;
  uint8_t *tmp = nullptr;
  uint32_t pitch = 0U;
  uint16_t bits_per_pixel = 0U;
  bool error = !BmpUtils::load_bmp_image(tmp, width_, height_, pitch,
                                         bits_per_pixel, image_path.c_str());
  data.reset(tmp);
  if (!error) {
    pixels_.resize(size(), 0);
    copy_raw_data(reinterpret_cast<T *>(data.get()));
  }
  return error;
}

template <> bool ImageBMP<uint8_t>::read(const std::string &image_path) {
  std::unique_ptr<uint8_t[]> data = nullptr;
  uint8_t *tmp = nullptr;
  bool error =
      !BmpUtils::load_bmp_image_8u(tmp, width_, height_, image_path.c_str());
  data.reset(tmp);
  if (!error) {
    pixels_.resize(size(), 0);
    copy_raw_data(data.get());
  }
  return error;
}

template <typename T> uint32_t ImageBMP<T>::width() const { return width_; }

template <typename T> uint32_t ImageBMP<T>::height() const { return height_; }

template <typename T> uint16_t ImageBMP<T>::number_of_channels() const {
  return bits_per_pixel() / bits_per_channel();
}

template <typename T> uint16_t ImageBMP<T>::bits_per_channel() const {
  return 8;
}

template <typename T> uint16_t ImageBMP<T>::bits_per_pixel() const {
  return std::numeric_limits<T>::digits;
}

template <typename T> size_t ImageBMP<T>::size() const {
  return static_cast<size_t>(width()) * static_cast<size_t>(height());
}

template <typename T> size_t ImageBMP<T>::size_in_bytes() const {
  return pixels_.size() * sizeof(T);
}

template <typename T>
T ImageBMP<T>::get_pixel(const uint32_t x, const uint32_t y) const {
  return pixels_[y * width() + x];
}

template <typename T>
void ImageBMP<T>::set_pixel(const uint32_t x, const uint32_t y, const T data) {
  pixels_[y * width() + x] = data;
}

template <typename T> std::vector<T> ImageBMP<T>::get_pixels() const {
  return pixels_;
}

template <typename T> void ImageBMP<T>::copy_raw_data(const T *data) {
  std::copy(data, data + size(), std::begin(pixels_));
}

template <typename T> T *ImageBMP<T>::raw_data() { return pixels_.data(); }

template <typename T> const T *ImageBMP<T>::raw_data() const {
  return pixels_.data();
}

template <typename T> bool ImageBMP<T>::operator==(const ImageBMP &rhs) const {
  return pixels_ == rhs.pixels_;
}

template class ImageBMP<uint8_t>;
template class ImageBMP<uint32_t>;
} // namespace level_zero_tests
