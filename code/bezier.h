#pragma once
#include "app.h"
#include "buffer.h"
#include "texture_image.h"

struct BezierGlobalUniformObject {
  glm::mat4 proj;
  glm::mat4 view;
  glm::vec3 control_points[5][5];
  float tess_level = 1.0f;
};

class Bezier : public Application {
 public:
  Bezier();

 private:
  void OnInitImpl() override;

  void OnShutdownImpl() override;

  void OnUpdateImpl() override;

  void OnRenderImpl(VkCommandBuffer cmd_buffer) override;

  void CreateAssets();
  void CreateDescriptorAssets();
  void CreatePipeline();
  void DestroyAssets();
  void DestroyDescriptorAssets();
  void DestroyPipeline();

  void OnKey(int key, int scancode, int action, int mods);

  void RandomizeControlPoints();

  std::shared_ptr<TextureImage> texture_image_;

  std::shared_ptr<vulkan::ShaderModule> vertex_shader_;
  std::shared_ptr<vulkan::ShaderModule> tess_control_shader_;
  std::shared_ptr<vulkan::ShaderModule> tess_evaluation_shader_;
  std::shared_ptr<vulkan::ShaderModule> fragment_shader_;
  std::shared_ptr<vulkan::ShaderModule> texture_fragment_shader_;

  std::shared_ptr<DynamicBuffer<BezierGlobalUniformObject>>
      global_uniform_buffer_;
  std::shared_ptr<vulkan::DescriptorSetLayout> descriptor_set_layout_;
  std::shared_ptr<vulkan::DescriptorPool> descriptor_pool_;
  std::vector<std::shared_ptr<vulkan::DescriptorSet>> descriptor_sets_;
  std::shared_ptr<vulkan::PipelineLayout> pipeline_layout_;
  std::shared_ptr<vulkan::Pipeline> pipeline_;
  std::shared_ptr<vulkan::Pipeline> texture_pipeline_;

  float rotation_phi_ = 0.0f;
  float rotation_theta_ = glm::radians(90.0f);

  float y_grid_[5][5]{};
  int tess_level_{20};
  bool wireframe_{false};
};
