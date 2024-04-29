#include "spiral.h"

namespace {
#include "built_in_shaders.inl"

glm::vec3 hsv2rgb(const glm::vec3 &hsv) {
  float h = hsv.x * 360.0f;  // scale hue to [0, 360)
  float s = hsv.y;
  float v = hsv.z;

  float c = v * s;  // chroma
  float x = c * (1.0f - std::fabs(std::fmod(h / 60.0f, 2) - 1.0f));
  float m = v - c;

  float r = 0.0f, g = 0.0f, b = 0.0f;

  if (h >= 0 && h < 60) {
    r = c, g = x, b = 0;
  } else if (h >= 60 && h < 120) {
    r = x, g = c, b = 0;
  } else if (h >= 120 && h < 180) {
    r = 0, g = c, b = x;
  } else if (h >= 180 && h < 240) {
    r = 0, g = x, b = c;
  } else if (h >= 240 && h < 300) {
    r = x, g = 0, b = c;
  } else if (h >= 300 && h < 360) {
    r = c, g = 0, b = x;
  }

  glm::vec3 rgb(r + m, g + m, b + m);
  return rgb;
}

}  // namespace

SpiralSystem::SpiralSystem() {
}

void SpiralSystem::OnInitImpl() {
  CreateAssets();
  CreateDescriptorAssets();
  CreatePipeline();
}

void SpiralSystem::OnShutdownImpl() {
  DestroyPipeline();
  DestroyDescriptorAssets();
  DestroyAssets();
}

void SpiralSystem::OnUpdateImpl() {
  static auto last_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  float duration_s = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - last_time)
                         .count();
  last_time = current_time;
  static float accumulated_time = 0.0f;
  static float phase_state = 0.0f;
  accumulated_time += duration_s;
  const float generate_duration = 0.05f;
  const float time_speed = 0.1f;
  while (accumulated_time >= generate_duration) {
    accumulated_time -= generate_duration;
    StarInfo star_info{};
    phase_state += glm::radians(15.0f);
    while (phase_state > glm::radians(360.0f)) {
      phase_state -= glm::radians(360.0f);
    }
    glm::vec3 hsv{phase_state / glm::radians(360.0f), 0.7f, 1.0f};
    star_info.color = hsv2rgb(hsv);
    star_info.phase = phase_state;
    star_info.life = accumulated_time * time_speed;
    star_infos_.push_back(star_info);
  }
  std::vector<Star> stars;
  std::vector<StarInfo> new_star_infos;
  for (auto &star_info : star_infos_) {
    star_info.life += time_speed * duration_s;
    if (star_info.life < 1.0f) {
      stars.push_back(star_info.GetStar());
      new_star_infos.push_back(star_info);
    }
  }
  star_infos_ = new_star_infos;
  std::memcpy(star_buffer_->Data(), stars.data(), sizeof(Star) * stars.size());
}

void SpiralSystem::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  VkDeviceSize offsets[] = {0};
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_->Handle());
  VkDescriptorSet descriptor_set = descriptor_sets_[CurrentFrame()]->Handle();
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_layout_->Handle(), 0, 1, &descriptor_set, 0,
                          nullptr);
  VkBuffer vertex_buffers[] = {star_buffer_->GetBuffer()->Handle()};
  vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdDraw(cmd_buffer, 6, star_infos_.size(), 0, 0);
}

void SpiralSystem::CreateAssets() {
  Image star_image_host;
  star_image_host.ReadFromFile(ASSETS_PATH "texture/Star.bmp");
  star_image_ = std::make_shared<TextureImage>(this, star_image_host);

  global_uniform_buffer_ = std::make_shared<StaticBuffer<glm::mat4>>(this, 1);

  star_buffer_ = std::make_shared<DynamicBuffer<Star>>(this, 1000);
  auto extent = Swapchain()->Extent();
  glm::mat4 transform = glm::mat4{1.0f};
  transform[0][0] = float(extent.height) / float(extent.width);
  global_uniform_buffer_->Upload({transform});
}

void SpiralSystem::DestroyAssets() {
  star_buffer_.reset();
  global_uniform_buffer_.reset();
  star_image_.reset();
}

void SpiralSystem::CreateDescriptorAssets() {
  VkDescriptorSetLayoutBinding global_uniform_binding{};
  global_uniform_binding.binding = 0;
  global_uniform_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  global_uniform_binding.descriptorCount = 1;
  global_uniform_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  global_uniform_binding.pImmutableSamplers = nullptr;

  VkSampler sampler = EntitySampler()->Handle();
  VkDescriptorSetLayoutBinding star_binding{};
  star_binding.binding = 1;
  star_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  star_binding.descriptorCount = 1;
  star_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  star_binding.pImmutableSamplers = &sampler;

  IgnoreResult(Device()->CreateDescriptorSetLayout(
      {global_uniform_binding, star_binding}, &descriptor_set_layout_));

  vulkan::DescriptorPoolSize pool_size =
      descriptor_set_layout_->GetPoolSize() * MaxFramesInFlight();
  IgnoreResult(
      Device()->CreateDescriptorPool(pool_size.ToVkDescriptorPoolSize(),
                                     MaxFramesInFlight(), &descriptor_pool_));

  descriptor_sets_.resize(MaxFramesInFlight());
  for (int i = 0; i < MaxFramesInFlight(); i++) {
    IgnoreResult(descriptor_pool_->AllocateDescriptorSet(
        descriptor_set_layout_->Handle(), &descriptor_sets_[i]));
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = global_uniform_buffer_->GetBuffer()->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(glm::mat4);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView = star_image_->GetImage()->ImageView();
    image_info.sampler = sampler;

    VkWriteDescriptorSet write{};
    std::vector<VkWriteDescriptorSet> writes{};

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

void SpiralSystem::DestroyDescriptorAssets() {
  descriptor_sets_.clear();
  descriptor_pool_.reset();
  descriptor_set_layout_.reset();
}

void SpiralSystem::CreatePipeline() {
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/star.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &vertex_shader_));

  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/star.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &fragment_shader_));

  IgnoreResult(Device()->CreatePipelineLayout(
      {descriptor_set_layout_->Handle()}, &pipeline_layout_));

  vulkan::PipelineSettings pipeline_settings(RenderPass(),
                                             pipeline_layout_.get());
  pipeline_settings.AddShaderStage(vertex_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(fragment_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  pipeline_settings.AddInputBinding(0, sizeof(Star),
                                    VK_VERTEX_INPUT_RATE_INSTANCE);
  pipeline_settings.AddInputAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT,
                                      offsetof(Star, position));
  pipeline_settings.AddInputAttribute(0, 1, VK_FORMAT_R32_SFLOAT,
                                      offsetof(Star, size));
  pipeline_settings.AddInputAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Star, color));

  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.SetMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);
  pipeline_settings.SetBlendState(
      0, {
             VK_TRUE,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_OP_ADD,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_OP_ADD,
             VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
         });
  pipeline_settings.depth_stencil_state_create_info->depthTestEnable = VK_FALSE;

  IgnoreResult(Device()->CreatePipeline(pipeline_settings, &pipeline_));
}

void SpiralSystem::DestroyPipeline() {
  pipeline_.reset();
  pipeline_layout_.reset();
  vertex_shader_.reset();
  fragment_shader_.reset();
}
