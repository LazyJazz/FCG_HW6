#pragma once
#include "model.h"
#include "texture_image.h"

class Entity {
 public:
  Entity(Application *app, Model *model, TextureImage *image);

  ~Entity() {
  }

  void Render(VkCommandBuffer cmd_buffer,
              VkPipelineLayout pipeline_layout) const;

 private:
  Application *app_;
  Model *model_;
  TextureImage *image_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
};
