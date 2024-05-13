#include "bezier.h"

#include "glm/gtc/matrix_transform.hpp"
#include "random"

namespace {
#include "built_in_shaders.inl"
}

Bezier::Bezier() {
  RandomizeControlPoints();
  glfwSetKeyCallback(Window(), [](GLFWwindow *window, int key, int scancode,
                                  int action, int mods) {
    auto app = reinterpret_cast<Bezier *>(glfwGetWindowUserPointer(window));
    app->OnKey(key, scancode, action, mods);
  });
  glfwSetWindowUserPointer(Window(), this);
}

void Bezier::OnInitImpl() {
  CreateAssets();
  CreateDescriptorAssets();
  CreatePipeline();
}

void Bezier::OnShutdownImpl() {
  DestroyPipeline();
  DestroyDescriptorAssets();
  DestroyAssets();
}

void Bezier::OnUpdateImpl() {
  static auto last_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  float duration_s = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - last_time)
                         .count();
  last_time = current_time;
  if (glfwGetKey(Window(), GLFW_KEY_A) == GLFW_PRESS) {
    rotation_phi_ += 0.5f * duration_s;
  }
  if (glfwGetKey(Window(), GLFW_KEY_D) == GLFW_PRESS) {
    rotation_phi_ -= 0.5f * duration_s;
  }
  if (glfwGetKey(Window(), GLFW_KEY_W) == GLFW_PRESS) {
    rotation_theta_ -= 0.5f * duration_s;
  }
  if (glfwGetKey(Window(), GLFW_KEY_S) == GLFW_PRESS) {
    rotation_theta_ += 0.5f * duration_s;
  }
  while (rotation_phi_ > 2 * glm::pi<float>()) {
    rotation_phi_ -= 2 * glm::pi<float>();
  }
  while (rotation_phi_ < 0.0f) {
    rotation_phi_ += 2 * glm::pi<float>();
  }
  if (rotation_theta_ > glm::pi<float>() - 1e-3f) {
    rotation_theta_ = glm::pi<float>() - 1e-3f;
  } else if (rotation_theta_ < 1e-3f) {
    rotation_theta_ = 1e-3f;
  }

  BezierGlobalUniformObject ubo{};
  ubo.proj =
      glm::perspective(glm::radians(45.0f),
                       static_cast<float>(Swapchain()->Extent().width) /
                           static_cast<float>(Swapchain()->Extent().height),
                       0.1f, 10.0f);
  float camera_dist = 3.0f;
  glm::vec3 camera_pos =
      glm::vec3(glm::sin(rotation_phi_) * glm::sin(rotation_theta_),
                glm::cos(rotation_theta_),
                glm::cos(rotation_phi_) * glm::sin(rotation_theta_)) *
      camera_dist;
  ubo.view = glm::lookAt(camera_pos, glm::vec3(0.0f, 0.0f, 0.0f),
                         glm::vec3(0.0f, 1.0f, 0.0f));
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      ubo.control_points[i][j] =
          glm::vec3(float(i - 2) * 0.5f, y_grid_[i][j], float(j - 2) * 0.5f);
    }
  }
  ubo.tess_level = tess_level_;
  global_uniform_buffer_->At(0) = ubo;
}

void Bezier::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(
      cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      wireframe_ ? pipeline_->Handle() : texture_pipeline_->Handle());

  VkDescriptorSet descriptor_set = descriptor_sets_[CurrentFrame()]->Handle();

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_->Handle(), 0, 1, &descriptor_set, 0,
                          nullptr);

  vkCmdDraw(cmd_buffer, 4, 1, 0, 0);
}

void Bezier::CreateAssets() {
  global_uniform_buffer_ =
      std::make_shared<DynamicBuffer<BezierGlobalUniformObject>>(this, 1);

  Image texture;
  texture.ReadFromFile(ASSETS_PATH "texture/texture.jpg");
  texture_image_ = std::make_shared<TextureImage>(this, texture);
}

void Bezier::DestroyAssets() {
  texture_image_.reset();
  global_uniform_buffer_.reset();
}

void Bezier::CreateDescriptorAssets() {
  IgnoreResult(Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
            VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        nullptr},
       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}},
      &descriptor_set_layout_));

  vulkan::DescriptorPoolSize pool_size =
      descriptor_set_layout_->GetPoolSize() * MaxFramesInFlight();
  IgnoreResult(
      Device()->CreateDescriptorPool(pool_size.ToVkDescriptorPoolSize(),
                                     MaxFramesInFlight(), &descriptor_pool_));

  descriptor_sets_.resize(MaxFramesInFlight());
  for (size_t i = 0; i < descriptor_sets_.size(); ++i) {
    IgnoreResult(descriptor_pool_->AllocateDescriptorSet(
        descriptor_set_layout_->Handle(), &descriptor_sets_[i]));
    std::vector<VkWriteDescriptorSet> writes;
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = global_uniform_buffer_->GetBuffer(i)->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(BezierGlobalUniformObject);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = texture_image_->GetImage()->ImageView();
    image_info.sampler = EntitySampler()->Handle();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_sets_[i]->Handle();
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &buffer_info;
    writes.push_back(write);

    write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_sets_[i]->Handle();
    write.dstBinding = 1;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &image_info;
    writes.push_back(write);

    vkUpdateDescriptorSets(Device()->Handle(), writes.size(), writes.data(), 0,
                           nullptr);
  }
}

void Bezier::DestroyDescriptorAssets() {
  descriptor_sets_.clear();
  descriptor_pool_.reset();
  descriptor_set_layout_.reset();
}

void Bezier::CreatePipeline() {
  IgnoreResult(Device()->CreatePipelineLayout(
      {descriptor_set_layout_->Handle()}, &pipeline_layout_));

  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/bezier.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &vertex_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/bezier.tesc"),
                                 VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT),
      &tess_control_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/bezier.tese"),
                                 VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT),
      &tess_evaluation_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/bezier.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &fragment_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/bezier_texture.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &texture_fragment_shader_));

  vulkan::PipelineSettings settings(RenderPass(), pipeline_layout_.get());
  settings.AddShaderStage(vertex_shader_.get(), VK_SHADER_STAGE_VERTEX_BIT);
  settings.AddShaderStage(tess_control_shader_.get(),
                          VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  settings.AddShaderStage(tess_evaluation_shader_.get(),
                          VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  settings.AddShaderStage(fragment_shader_.get(), VK_SHADER_STAGE_FRAGMENT_BIT);
  settings.SetCullMode(VK_CULL_MODE_NONE);
  settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
  settings.SetPolygonMode(VK_POLYGON_MODE_LINE);
  settings.SetTessellationState(4);

  IgnoreResult(Device()->CreatePipeline(settings, &pipeline_));

  vulkan::PipelineSettings texture_settings(RenderPass(),
                                            pipeline_layout_.get());
  texture_settings.AddShaderStage(vertex_shader_.get(),
                                  VK_SHADER_STAGE_VERTEX_BIT);
  texture_settings.AddShaderStage(tess_control_shader_.get(),
                                  VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
  texture_settings.AddShaderStage(tess_evaluation_shader_.get(),
                                  VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
  texture_settings.AddShaderStage(texture_fragment_shader_.get(),
                                  VK_SHADER_STAGE_FRAGMENT_BIT);
  texture_settings.SetCullMode(VK_CULL_MODE_NONE);
  texture_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
  texture_settings.SetPolygonMode(VK_POLYGON_MODE_FILL);
  texture_settings.SetTessellationState(4);
  IgnoreResult(Device()->CreatePipeline(texture_settings, &texture_pipeline_));
}

void Bezier::DestroyPipeline() {
  texture_pipeline_.reset();
  pipeline_.reset();
  vertex_shader_.reset();
  tess_control_shader_.reset();
  tess_evaluation_shader_.reset();
  fragment_shader_.reset();
  texture_fragment_shader_.reset();
  pipeline_layout_.reset();
}

void Bezier::RandomizeControlPoints() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(-0.5f, 0.5f);
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      y_grid_[i][j] =
          dis(gen) * std::min(5 - i, i + 1) * std::min(5 - j, j + 1);
    }
  }
}

void Bezier::OnKey(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_TAB) {
      wireframe_ = !wireframe_;
    }
    if (key == GLFW_KEY_SPACE) {
      RandomizeControlPoints();
    }
    if (key == GLFW_KEY_UP) {
      tess_level_++;
    }
    if (key == GLFW_KEY_DOWN) {
      tess_level_--;
    }
  }
  if (tess_level_ < 3) {
    tess_level_ = 3;
  } else if (tess_level_ > 50) {
    tess_level_ = 50;
  }
}
