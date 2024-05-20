#pragma once
#include "app.h"
#include "buffer.h"
#include "entity.h"

struct LightingGlobalUniformObject {
  glm::mat4 proj;
  glm::mat4 world;
  glm::vec4 directional_light_direction;
  glm::vec4 directional_light_color;
  glm::vec4 specular_light;
  glm::vec4 ambient_light_color;
};

class Lighting : public Application {
 public:
  Lighting();

 private:
  void OnInitImpl() override;
  void OnShutdownImpl() override;
  void OnUpdateImpl() override;
  void OnRenderImpl(VkCommandBuffer cmd_buffer) override;

  void CreateEntityPipelineAssets();
  void CreateGlobalAssets();
  void CreateEntities();

  void DestroyEntityPipelineAssets();
  void DestroyGlobalAssets();
  void DestroyEntities();

  void OnKeyEvent(int key, int scancode, int action, int mods);
  void OnScroll(double xoffset, double yoffset);

  std::shared_ptr<vulkan::ShaderModule> entity_vert_shader_;
  std::shared_ptr<vulkan::ShaderModule> entity_frag_shader_;
  std::shared_ptr<vulkan::PipelineLayout> entity_pipeline_layout_;
  std::shared_ptr<vulkan::Pipeline> entity_pipeline_;

  std::unique_ptr<Model> model_;
  std::unique_ptr<Model> face_model_;

  std::unique_ptr<Entity> entity_;
  std::unique_ptr<Entity> face_entity_;

  std::unique_ptr<TextureImage> white_texture_;

  std::unique_ptr<vulkan::DescriptorSetLayout> global_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> global_descriptor_pool_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> global_descriptor_sets_;
  std::unique_ptr<DynamicBuffer<LightingGlobalUniformObject>>
      global_uniform_buffer_;

  LightingGlobalUniformObject global_uniform_object_;

  float light_theta_{glm::radians(60.0f)};
  float light_phi_{glm::radians(30.0f)};
  float light_h_{0.0f};

  float camera_theta_{};
  float camera_phi_{};

  glm::vec3 camera_position_{0.0f, 0.0f, 2.0f};
  bool smoothed_model_{true};

  glm::mat4 model_transform_{1.0f};
};
