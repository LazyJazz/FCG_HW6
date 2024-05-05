#pragma once
#include "app.h"
#include "buffer.h"
#include "random"
#include "texture_image.h"

struct Snow {
  glm::vec2 position;
  float size;
  float alpha;
};

struct SnowInfo {
  glm::vec2 position;
  float size;
  float alpha;
  glm::vec2 velocity;
  Snow GetSnow() const {
    return {position, size, alpha};
  }
};

class SnowSystem : public Application {
 public:
  SnowSystem();

 private:
  void OnInitImpl() override;

  void OnUpdateImpl() override;

  void OnRenderImpl(VkCommandBuffer cmd_buffer) override;

  void OnShutdownImpl() override;

  void CreateAssets();
  void CreateDescriptorAssets();
  void CreatePipeline();

  void DestroyAssets();
  void DestroyDescriptorAssets();
  void DestroyPipeline();

  std::shared_ptr<TextureImage> background_image_;
  std::shared_ptr<TextureImage> snow_particle_image_;
  std::shared_ptr<StaticBuffer<glm::mat4>> global_uniform_buffer_;
  std::shared_ptr<DynamicBuffer<Snow>> snow_buffer_;

  std::shared_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::shared_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::shared_ptr<vulkan::DescriptorSet> descriptor_set_;

  std::shared_ptr<vulkan::PipelineLayout> pipeline_layout_;

  std::shared_ptr<vulkan::ShaderModule> vertex_shader_;
  std::shared_ptr<vulkan::ShaderModule> fragment_shader_;
  std::shared_ptr<vulkan::Pipeline> pipeline_;

  std::shared_ptr<vulkan::ShaderModule> background_vertex_shader_;
  std::shared_ptr<vulkan::ShaderModule> background_fragment_shader_;
  std::shared_ptr<vulkan::Pipeline> background_pipeline_;

  std::vector<SnowInfo> snow_infos_;
  std::random_device rd_;
};
