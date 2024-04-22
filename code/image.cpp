#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void Image::ReadFromFile(const std::string &path) {
  int width, height, channels;
  unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (!data) {
    throw std::runtime_error("Failed to load image: " + path);
  }
  width_ = width;
  height_ = height;
  pixels_.resize(width * height);
  std::memcpy(pixels_.data(), data, width * height * sizeof(ImagePixel));
  stbi_image_free(data);
}

void Image::WriteToFile(const std::string &path) const {
  // Judge if ends with .png
  if (path.size() >= 4 && path.substr(path.size() - 4) == ".png") {
    stbi_write_png(path.c_str(), width_, height_, 4, pixels_.data(),
                   width_ * 4);
  } else if (path.size() >= 4 && path.substr(path.size() - 4) == ".bmp") {
    stbi_write_bmp(path.c_str(), width_, height_, 4, pixels_.data());
  } else if (path.size() >= 4 && path.substr(path.size() - 4) == ".tga") {
    stbi_write_tga(path.c_str(), width_, height_, 4, pixels_.data());
  } else if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg") {
    stbi_write_jpg(path.c_str(), width_, height_, 4, pixels_.data(), 100);
  } else {
    throw std::runtime_error("Unsupported image format: " + path);
  }
}
