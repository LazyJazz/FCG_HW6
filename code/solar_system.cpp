#include "solar_system.h"

#include "celestial_body.h"

namespace {
#include "built_in_shaders.inl"
}

void SolarSystem::OnInitImpl() {
  CreateGlobalAssets();
  CreateEntityPipelineAssets();
  CreateEntities();
  CreateCelestialBodies();
}

void SolarSystem::OnShutdownImpl() {
  DestroyCelestialBodies();
  DestroyEntities();
  DestroyEntityPipelineAssets();
  DestroyGlobalAssets();
}

#define THROW_IF_FAILED(x, err_msg)    \
  result = (x);                        \
  if (result != VK_SUCCESS) {          \
    throw std::runtime_error(err_msg); \
  }

void SolarSystem::OnUpdateImpl() {
  auto extent = Swapchain()->Extent();
  float aspect = extent.width / static_cast<float>(extent.height);
  global_uniform_object_.proj =
      glm::perspective(glm::radians(45.0f), aspect, 0.1f, 40.0f);
  global_uniform_object_.world =
      glm::lookAt(glm::vec3{0.0f, 0.0f, 15.0f}, glm::vec3{0.0f, 0.0f, 0.0f},
                  glm::vec3{0.0f, 1.0f, 0.0f});
  global_uniform_buffer_->At(0) = global_uniform_object_;
}

void SolarSystem::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    entity_pipeline_->Handle());

  VkDescriptorSet descriptor_sets[] = {
      global_descriptor_sets_[CurrentFrame()]->Handle()};
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          entity_pipeline_layout_->Handle(), 0, 1,
                          descriptor_sets, 0, nullptr);

  //  triangle_entity_->Render(cmd_buffer, entity_pipeline_layout_->Handle());
  for (auto planet : planets_) {
    planet->Render(cmd_buffer);
  }
}

void SolarSystem::CreateEntityPipelineAssets() {
  IgnoreResult(
      Device()->CreatePipelineLayout({global_descriptor_set_layout_->Handle(),
                                      EntityDescriptorSetLayout()->Handle()},
                                     &entity_pipeline_layout_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &entity_vert_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &entity_frag_shader_));

  vulkan::PipelineSettings pipeline_settings(RenderPass(),
                                             entity_pipeline_layout_.get(), 0);
  pipeline_settings.AddInputBinding(0, sizeof(Vertex),
                                    VK_VERTEX_INPUT_RATE_VERTEX);
  pipeline_settings.AddInputAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, pos));
  pipeline_settings.AddInputAttribute(0, 1, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, normal));
  pipeline_settings.AddInputAttribute(0, 2, VK_FORMAT_R32G32B32_SFLOAT,
                                      offsetof(Vertex, color));
  pipeline_settings.AddInputAttribute(0, 3, VK_FORMAT_R32G32_SFLOAT,
                                      offsetof(Vertex, tex_coord));
  pipeline_settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipeline_settings.SetCullMode(VK_CULL_MODE_NONE);
  pipeline_settings.AddShaderStage(entity_vert_shader_.get(),
                                   VK_SHADER_STAGE_VERTEX_BIT);
  pipeline_settings.AddShaderStage(entity_frag_shader_.get(),
                                   VK_SHADER_STAGE_FRAGMENT_BIT);
  IgnoreResult(Device()->CreatePipeline(pipeline_settings, &entity_pipeline_));
}

void SolarSystem::DestroyEntityPipelineAssets() {
  entity_pipeline_.reset();
  entity_vert_shader_.reset();
  entity_frag_shader_.reset();
  entity_pipeline_layout_.reset();
}

void SolarSystem::CreateEntities() {
  std::vector<Vertex> vertices = {{{-0.5f, -0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {1.0f, 1.0f, 1.0f},
                                   {0.0f, 0.0f}},
                                  {{0.5f, -0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {1.0f, 1.0f, 1.0f},
                                   {1.0f, 0.0f}},
                                  {{-0.5f, 0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {1.0f, 1.0f, 1.0f},
                                   {0.0f, 1.0f}},
                                  {{0.5f, 0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {1.0f, 1.0f, 1.0f},
                                   {1.0f, 1.0f}}};
  std::vector<uint32_t> indices = {0, 1, 2, 2, 1, 3};
  Image image;
  image.ReadFromFile(ASSETS_PATH "texture/earth.jpg");

  triangle_ = std::make_unique<Model>(this, vertices, indices);
  triangle_texture_image_ = std::make_unique<TextureImage>(this, image);
  triangle_entity_ = std::make_unique<Entity>(this, triangle_.get(),
                                              triangle_texture_image_.get());

  const int precision = 30;
  const float inv_precision = 1.0f / float(precision);
  std::vector<Vertex> sphere_vertices;
  std::vector<uint32_t> sphere_indices;

  for (int i = 0; i <= precision; i++) {
    float phi = glm::pi<float>() * float(i) * inv_precision;
    for (int j = 0; j < precision; j++) {
      float theta = 2.0f * glm::pi<float>() * float(j) * inv_precision;
      glm::vec3 pos = glm::vec3{glm::cos(theta) * glm::sin(phi), glm::cos(phi),
                                glm::sin(theta) * glm::sin(phi)};
      glm::vec3 normal = glm::normalize(pos);
      glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f};
      glm::vec2 tex_coord =
          glm::vec2{float(j) * inv_precision, float(i) * inv_precision};
      sphere_vertices.push_back({pos, normal, color, tex_coord});

      if (i) {
        int i1 = (i - 1 + precision) % precision;
        int j1 = (j + 1 + precision) % precision;
        sphere_indices.push_back(i1 * precision + j);
        sphere_indices.push_back(i1 * precision + j1);
        sphere_indices.push_back(i * precision + j);
        sphere_indices.push_back(i1 * precision + j1);
        sphere_indices.push_back(i * precision + j1);
        sphere_indices.push_back(i * precision + j);
      }
    }
  }

  sphere_ = std::make_unique<Model>(this, sphere_vertices, sphere_indices);
}

void SolarSystem::DestroyEntities() {
  sphere_.reset();

  triangle_texture_image_.reset();
  triangle_entity_.reset();
  triangle_.reset();
}

void SolarSystem::CreateGlobalAssets() {
  global_uniform_buffer_ =
      std::make_unique<DynamicBuffer<GlobalUniformObject>>(this, 1);
  VkResult result;
  THROW_IF_FAILED(Device()->CreateDescriptorSetLayout(
                      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                        VK_SHADER_STAGE_VERTEX_BIT, nullptr}},
                      &global_descriptor_set_layout_),
                  "Failed to create global descriptor set layout.")

  vulkan::DescriptorPoolSize pool_size =
      (global_descriptor_set_layout_->GetPoolSize()) * MaxFramesInFlight();
  THROW_IF_FAILED(Device()->CreateDescriptorPool(
                      pool_size.ToVkDescriptorPoolSize(), MaxFramesInFlight(),
                      &global_descriptor_pool_),
                  "Failed to create global descriptor pool.")

  global_descriptor_sets_.resize(MaxFramesInFlight());
  for (int i = 0; i < MaxFramesInFlight(); i++) {
    THROW_IF_FAILED(global_descriptor_pool_->AllocateDescriptorSet(
                        global_descriptor_set_layout_->Handle(),
                        &global_descriptor_sets_[i]),
                    "Failed to allocate global descriptor set.")

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = global_uniform_buffer_->GetBuffer(i)->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(GlobalUniformObject);

    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext = nullptr;
    write_descriptor_set.dstSet = global_descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(Device()->Handle(), 1, &write_descriptor_set, 0,
                           nullptr);
  }
}

void SolarSystem::DestroyGlobalAssets() {
  global_descriptor_sets_.clear();
  global_descriptor_pool_.reset();
  global_descriptor_set_layout_.reset();
  global_uniform_buffer_.reset();
}

void SolarSystem::CreateCelestialBodies() {
  sun_ = std::make_unique<CelestialBody>(this, nullptr, 1.0f, 1.0f, 0.0f, 1.0f,
                                         0.0f, 0.0f,
                                         ASSETS_PATH "texture/sun.jpg");
  planets_.push_back(sun_.get());
}

void SolarSystem::DestroyCelestialBodies() {
  sun_.reset();
}
