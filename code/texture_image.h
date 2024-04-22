#pragma once
#include "app.h"
#include "image.h"

class TextureImage {
 public:
  TextureImage(Application *app, const Image &image);

  [[nodiscard]] vulkan::Image *GetImage() const {
    return image_.get();
  }

 private:
  Application *app_{};
  std::unique_ptr<vulkan::Image> image_;
};
