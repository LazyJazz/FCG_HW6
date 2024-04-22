#include "solar_system.h"

namespace {
#include "built_in_shaders.inl"
}

void SolarSystem::OnInitImpl() {
  CreateEntityPipelineAssets();
  CreateEntities();
}

void SolarSystem::OnShutdownImpl() {
  DestroyEntities();
  DestroyEntityPipelineAssets();
}

void SolarSystem::OnUpdateImpl() {
}

void SolarSystem::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_->Handle());

  triangle_entity_->Render(cmd_buffer);
}

void SolarSystem::CreateEntityPipelineAssets() {
  IgnoreResult(Device()->CreatePipelineLayout({}, &pipeline_layout_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &entity_vert_shader_));
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/entity.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &entity_frag_shader_));

  vulkan::PipelineSettings pipeline_settings(RenderPass(),
                                             pipeline_layout_.get(), 0);
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
  IgnoreResult(Device()->CreatePipeline(pipeline_settings, &pipeline_));
}

void SolarSystem::DestroyEntityPipelineAssets() {
  pipeline_.reset();
  entity_vert_shader_.reset();
  entity_frag_shader_.reset();
  pipeline_layout_.reset();
}

void SolarSystem::CreateEntities() {
  std::vector<Vertex> vertices = {{{0.0f, 0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f}},
                                  {{-0.5f, -0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f}},
                                  {{0.5f, -0.5f, 0.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {0.0f, 0.0f, 1.0f},
                                   {0.0f, 0.0f}}};
  std::vector<uint32_t> indices = {0, 1, 2};

  triangle_ = std::make_unique<Model>(this, vertices, indices);
  triangle_entity_ = std::make_unique<Entity>(this, triangle_.get(), nullptr);
}

void SolarSystem::DestroyEntities() {
  triangle_entity_.reset();
  triangle_.reset();
}
