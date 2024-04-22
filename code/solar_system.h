#pragma once
#include "app.h"
#include "buffer.h"
#include "entity.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "model.h"

struct GlobalUniform {
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
  void CreateEntities();

  void DestroyEntities();
  void DestroyEntityPipelineAssets();

  std::shared_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::shared_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::shared_ptr<vulkan::DescriptorSet> descriptor_set_;

  std::shared_ptr<vulkan::ShaderModule> entity_vert_shader_;
  std::shared_ptr<vulkan::ShaderModule> entity_frag_shader_;
  std::shared_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::shared_ptr<vulkan::Pipeline> pipeline_;

  std::unique_ptr<Model> triangle_;
  std::unique_ptr<Entity> triangle_entity_;
};
