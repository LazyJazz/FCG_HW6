#pragma once
#include "app.h"
#include "buffer.h"
#include "texture_image.h"

struct Star {
  glm::vec2 position;
  float size;
  glm::vec3 color;
};

struct StarInfo {
  glm::vec3 color;
  float phase;
  float life;
  Star GetStar() const {
    float sin_phase = sin(phase);
    float cos_phase = cos(phase);
    float length = 0.15f * 0.25f * std::exp(life * 15.0f);
    return {glm::vec2{sin_phase, cos_phase} * length, 0.15f, color};
  }
};

class SpiralSystem : public Application {
 public:
  SpiralSystem();

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

  std::shared_ptr<TextureImage> star_image_;
  std::shared_ptr<StaticBuffer<glm::mat4>> global_uniform_buffer_;
  std::shared_ptr<DynamicBuffer<Star>> star_buffer_;

  std::shared_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::shared_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::vector<std::shared_ptr<vulkan::DescriptorSet>> descriptor_sets_;

  std::shared_ptr<vulkan::ShaderModule> vertex_shader_;
  std::shared_ptr<vulkan::ShaderModule> fragment_shader_;
  std::shared_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::shared_ptr<vulkan::Pipeline> pipeline_;

  std::vector<StarInfo> star_infos_;
};
