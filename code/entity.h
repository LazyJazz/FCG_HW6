#pragma once
#include "model.h"
#include "texture_image.h"

struct EntityUniformObject {
  glm::mat4 model_{};
  glm::vec4 color_{1.0f};
};

class Entity {
 public:
  Entity(Application *app, Model *model, TextureImage *image);

  ~Entity() {
  }

  void Render(VkCommandBuffer cmd_buffer,
              VkPipelineLayout pipeline_layout) const;

  void SetEntityInfo(const EntityUniformObject &entity_info) const;

 private:
  Application *app_;
  Model *model_;
  TextureImage *image_;
  std::unique_ptr<DynamicBuffer<EntityUniformObject>> entity_uniform_object_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> descriptor_sets_;
};
