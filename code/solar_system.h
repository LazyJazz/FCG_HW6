#pragma once
#include "app.h"
#include "buffer.h"
#include "entity.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "model.h"
#include "texture_image.h"

struct GlobalUniformObject {
  glm::mat4 proj;
  glm::mat4 world;
};

class SolarSystem : public Application {
 public:
 private:
  void OnInitImpl() override;
  void OnUpdateImpl() override;
  void OnRenderImpl(VkCommandBuffer cmd_buffer) override;
  void OnShutdownImpl() override;

  void CreateEntityPipelineAssets();
  void CreateGlobalAssets();
  void CreateEntities();

  void DestroyEntityPipelineAssets();
  void DestroyGlobalAssets();
  void DestroyEntities();

  std::shared_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::shared_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::shared_ptr<vulkan::DescriptorSet> descriptor_set_;

  std::shared_ptr<vulkan::ShaderModule> entity_vert_shader_;
  std::shared_ptr<vulkan::ShaderModule> entity_frag_shader_;
  std::shared_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::shared_ptr<vulkan::Pipeline> pipeline_;

  std::unique_ptr<Model> triangle_;
  std::unique_ptr<TextureImage> triangle_texture_image_;
  std::unique_ptr<Entity> triangle_entity_;

  std::unique_ptr<vulkan::DescriptorSetLayout> global_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> global_descriptor_pool_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> global_descriptor_sets_;
  std::unique_ptr<DynamicBuffer<GlobalUniformObject>> global_uniform_buffer_;

  GlobalUniformObject global_uniform_object_;
};
