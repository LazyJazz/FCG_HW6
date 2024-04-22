#pragma once
#include "image.h"
#include "model.h"

class Entity {
 public:
  Entity(Application *app, Model *model, Image *image)
      : app_(app), model_(model), image_(image) {
  }

  ~Entity() {
  }

  void Render(VkCommandBuffer cmd_buffer) const;

 private:
  Application *app_;
  Model *model_;
  Image *image_;
};
