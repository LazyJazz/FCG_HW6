#pragma once
#include "utils.h"

struct ImagePixel {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

class Image {
 public:
  Image(size_t width = 1, size_t height = 1, ImagePixel pixel = ImagePixel{})
      : width_(width), height_(height), pixels_(width * height, pixel) {
  }

  Image(size_t width, size_t height, uint8_t *data)
      : width_(width), height_(height), pixels_(width * height) {
    for (size_t i = 0; i < width * height; ++i) {
      pixels_[i].r = data[i];
      pixels_[i].g = data[i];
      pixels_[i].b = data[i];
      pixels_[i].a = data[i];
    }
  }

  [[nodiscard]] size_t Width() const {
    return width_;
  }
  [[nodiscard]] size_t Height() const {
    return height_;
  }

  std::vector<ImagePixel> &Pixels() {
    return pixels_;
  }
  [[nodiscard]] const std::vector<ImagePixel> &Pixels() const {
    return pixels_;
  }

  ImagePixel *Data() {
    return pixels_.data();
  }
  [[nodiscard]] const ImagePixel *Data() const {
    return pixels_.data();
  }

  ImagePixel &operator()(size_t x, size_t y) {
    return pixels_[y * width_ + x];
  }

  [[nodiscard]] const ImagePixel &operator()(size_t x, size_t y) const {
    return pixels_[y * width_ + x];
  }

  void ReadFromFile(const std::string &path);

  void WriteToFile(const std::string &path) const;

 private:
  std::vector<ImagePixel> pixels_;
  size_t width_;
  size_t height_;
};
