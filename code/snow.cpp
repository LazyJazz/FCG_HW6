#include "snow.h"

namespace {
#include "built_in_shaders.inl"
}

SnowSystem::SnowSystem() : rd_() {
}

void SnowSystem::OnInitImpl() {
  CreateAssets();
  CreateDescriptorAssets();
  CreatePipeline();
}

void SnowSystem::OnShutdownImpl() {
  DestroyPipeline();
  DestroyDescriptorAssets();
  DestroyAssets();
}

void SnowSystem::OnUpdateImpl() {
  static auto last_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  float duration_s = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - last_time)
                         .count();
  last_time = current_time;
  static float accumulated_time = 0.0f;
  accumulated_time += duration_s;
  static float generate_duration = 0.5f;
  static float duration_scalar = 3.0f;
  auto extent = Swapchain()->Extent();
  float aspect = float(extent.width) / float(extent.height);
  while (accumulated_time > generate_duration) {
    SnowInfo snow_info{};
    snow_info.position = {
        std::uniform_real_distribution<float>(-aspect, aspect)(rd_), 1.0f};
    snow_info.size = std::uniform_real_distribution<float>(0.05f, 0.25f)(rd_);
    snow_info.position.y += snow_info.size;
    snow_info.alpha = std::uniform_real_distribution<float>(0.5f, 1.0f)(rd_);
    snow_info.velocity = {
        0.0f, -std::uniform_real_distribution<float>(0.1f, 0.5f)(rd_)};
    snow_infos_.push_back(snow_info);
    accumulated_time -= generate_duration;
    generate_duration = std::uniform_real_distribution<float>(0.1f, 0.5f)(rd_) *
                        duration_scalar;
    duration_scalar *= 0.95f;
    if (duration_scalar < 0.5f) {
      duration_scalar = 0.5f;
    }
  }

  std::vector<SnowInfo> new_snow_infos;
  std::vector<Snow> snows;
  for (auto snow_info : snow_infos_) {
    snow_info.position += snow_info.velocity * duration_s;
    if (snow_info.position.y < -1.0f) {
      continue;
    }
    snow_info.velocity.y -= 0.1f * duration_s;
    if (snow_info.velocity.y < -1.0f) {
      snow_info.velocity.y = -1.0f;
    }
    snows.push_back(snow_info.GetSnow());
    new_snow_infos.push_back(snow_info);
  }

  snow_infos_ = new_snow_infos;

  std::memcpy(snow_buffer_->Data(), snows.data(), sizeof(Snow) * snows.size());
}

void SnowSystem::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    background_pipeline_->Handle());
  VkDescriptorSet descriptor_sets[] = {descriptor_set_->Handle()};
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_->Handle(), 0, 1, descriptor_sets, 0,
                          nullptr);
  vkCmdDraw(cmd_buffer, 6, 1, 0, 0);

  VkDeviceSize offsets[] = {0};
  VkBuffer vertex_buffers[] = {snow_buffer_->GetBuffer()->Handle()};
  vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_->Handle());
  VkDescriptorSet descriptor_set = descriptor_set_->Handle();
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_->Handle(), 0, 1, &descriptor_set, 0,
                          nullptr);
  vkCmdDraw(cmd_buffer, 6, snow_infos_.size(), 0, 0);
}

void SnowSystem::CreateAssets() {
  Image snow_particle;
  snow_particle.ReadFromFile(ASSETS_PATH "texture/snow2.png");
  for (auto &pixel : snow_particle.Pixels()) {
    if (pixel.r == 255 && pixel.g == 255 && pixel.b == 255) {
      pixel.r = 0;
      pixel.g = 0;
      pixel.b = 0;
      pixel.a = 0;
    } else {
      pixel.a = 255;
    }
  }
  Image background;
  background.ReadFromFile(ASSETS_PATH "texture/background.jpg");
  snow_particle_image_ = std::make_shared<TextureImage>(this, snow_particle);
  background_image_ = std::make_shared<TextureImage>(this, background);

  snow_buffer_ = std::make_shared<DynamicBuffer<Snow>>(this, 1024);
  global_uniform_buffer_ = std::make_shared<StaticBuffer<glm::mat4>>(this, 1);
  auto extent = Swapchain()->Extent();
  glm::mat4 transform = glm::mat4{1.0f};
  transform[0][0] = float(extent.height) / float(extent.width);
  global_uniform_buffer_->Upload({transform});
}

void SnowSystem::DestroyAssets() {
  global_uniform_buffer_.reset();
  snow_buffer_.reset();
  background_image_.reset();
  snow_particle_image_.reset();
}

void SnowSystem::CreateDescriptorAssets() {
  VkSampler sampler = EntitySampler()->Handle();
  IgnoreResult(Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT,
        nullptr},
       {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
       {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}},
      &descriptor_set_layout_));
  IgnoreResult(Device()->CreateDescriptorPool(
      descriptor_set_layout_->GetPoolSize().ToVkDescriptorPoolSize(), 1,
      &descriptor_pool_));
  IgnoreResult(descriptor_pool_->AllocateDescriptorSet(
      descriptor_set_layout_->Handle(), &descriptor_set_));
  VkDescriptorBufferInfo buffer_info{};
  buffer_info.buffer = global_uniform_buffer_->GetBuffer()->Handle();
  buffer_info.offset = 0;
  buffer_info.range = sizeof(glm::mat4);
  VkDescriptorImageInfo image_info{};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = background_image_->GetImage()->ImageView();
  image_info.sampler = sampler;
  VkDescriptorImageInfo image_info2{};
  image_info2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info2.imageView = snow_particle_image_->GetImage()->ImageView();
  image_info2.sampler = sampler;
  VkWriteDescriptorSet write_descriptor_sets[3]{};
  write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_sets[0].dstSet = descriptor_set_->Handle();
  write_descriptor_sets[0].dstBinding = 0;
  write_descriptor_sets[0].dstArrayElement = 0;
  write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  write_descriptor_sets[0].descriptorCount = 1;
  write_descriptor_sets[0].pBufferInfo = &buffer_info;
  write_descriptor_sets[0].pImageInfo = nullptr;
  write_descriptor_sets[0].pTexelBufferView = nullptr;
  write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_sets[1].dstSet = descriptor_set_->Handle();
  write_descriptor_sets[1].dstBinding = 1;
  write_descriptor_sets[1].dstArrayElement = 0;
  write_descriptor_sets[1].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_sets[1].descriptorCount = 1;
  write_descriptor_sets[1].pBufferInfo = nullptr;
  write_descriptor_sets[1].pImageInfo = &image_info;
  write_descriptor_sets[1].pTexelBufferView = nullptr;
  write_descriptor_sets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_descriptor_sets[2].dstSet = descriptor_set_->Handle();
  write_descriptor_sets[2].dstBinding = 2;
  write_descriptor_sets[2].dstArrayElement = 0;
  write_descriptor_sets[2].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write_descriptor_sets[2].descriptorCount = 1;
  write_descriptor_sets[2].pBufferInfo = nullptr;
  write_descriptor_sets[2].pImageInfo = &image_info2;
  write_descriptor_sets[2].pTexelBufferView = nullptr;
  vkUpdateDescriptorSets(Device()->Handle(), 3, write_descriptor_sets, 0,
                         nullptr);
}

void SnowSystem::DestroyDescriptorAssets() {
  descriptor_set_.reset();
  descriptor_pool_.reset();
  descriptor_set_layout_.reset();
}

void SnowSystem::CreatePipeline() {
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/snow.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &vertex_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/snow.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &fragment_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/background.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &background_vertex_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/background.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &background_fragment_shader_));
  IgnoreResult(Device()->CreatePipelineLayout(
      {descriptor_set_layout_->Handle()}, &pipeline_layout_));
  vulkan::PipelineSettings pipeline_settings(RenderPass(),
                                             pipeline_layout_.get());
  pipeline_settings.AddShaderStage(vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  pipeline_settings.AddInputBinding(0, sizeof(Snow),
                                    VK_VERTEX_INPUT_RATE_INSTANCE);
  pipeline_settings.AddInputAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT,
                                      offsetof(Snow, position));
  pipeline_settings.AddInputAttribute(0, 1, VK_FORMAT_R32_SFLOAT,
                                      offsetof(Snow, size));
  pipeline_settings.AddInputAttribute(0, 2, VK_FORMAT_R32_SFLOAT,
                                      offsetof(Snow, alpha));
  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.depth_stencil_state_create_info->depthTestEnable = VK_FALSE;
  pipeline_settings.depth_stencil_state_create_info->depthWriteEnable =
      VK_FALSE;

  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);
  pipeline_settings.SetBlendState(
      0, {VK_TRUE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
          VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
          VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT});

  IgnoreResult(Device()->CreatePipeline(pipeline_settings, &pipeline_));

  vulkan::PipelineSettings background_pipeline_settings(RenderPass(),
                                                        pipeline_layout_.get());
  background_pipeline_settings.AddShaderStage(background_vertex_shader_.get(),
                                              VK_SHADER_STAGE_VERTEX_BIT);
  background_pipeline_settings.AddShaderStage(background_fragment_shader_.get(),
                                              VK_SHADER_STAGE_FRAGMENT_BIT);
  background_pipeline_settings.SetPrimitiveTopology(
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  background_pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);

  IgnoreResult(Device()->CreatePipeline(background_pipeline_settings,
                                        &background_pipeline_));
}

void SnowSystem::DestroyPipeline() {
  background_pipeline_.reset();
  pipeline_.reset();
  pipeline_layout_.reset();
  fragment_shader_.reset();
  vertex_shader_.reset();
  background_fragment_shader_.reset();
  background_vertex_shader_.reset();
}
