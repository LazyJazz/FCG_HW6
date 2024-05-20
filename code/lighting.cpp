#include "lighting.h"

#include "glm/gtc/matrix_transform.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

Lighting::Lighting() {
  glfwSetWindowUserPointer(Window(), this);
  glfwSetKeyCallback(Window(), [](GLFWwindow *window, int key, int scancode,
                                  int action, int mods) {
    auto app = reinterpret_cast<Lighting *>(glfwGetWindowUserPointer(window));
    app->OnKeyEvent(key, scancode, action, mods);
  });
  glfwSetScrollCallback(Window(), [](GLFWwindow *window, double xoffset,
                                     double yoffset) {
    auto app = reinterpret_cast<Lighting *>(glfwGetWindowUserPointer(window));
    app->OnScroll(xoffset, yoffset);
  });

  model_transform_ = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                 glm::vec3(1.0f, 0.0f, 0.0f)) *
                     glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
}

void Lighting::OnInitImpl() {
  CreateGlobalAssets();
  CreateEntityPipelineAssets();
  CreateEntities();
}

void Lighting::OnShutdownImpl() {
  DestroyEntities();
  DestroyEntityPipelineAssets();
  DestroyGlobalAssets();
}

void Lighting::OnUpdateImpl() {
  static auto last_time = std::chrono::high_resolution_clock::now();
  auto current_time = std::chrono::high_resolution_clock::now();
  float duration_s = std::chrono::duration<float, std::chrono::seconds::period>(
                         current_time - last_time)
                         .count();
  last_time = current_time;

  double cur_x, cur_y;
  glfwGetCursorPos(Window(), &cur_x, &cur_y);
  static double last_x = cur_x, last_y = cur_y;
  double diff_x = cur_x - last_x, diff_y = cur_y - last_y;
  last_x = cur_x;
  last_y = cur_y;

  const float mouse_speed = 0.001f;
  if (glfwGetMouseButton(Window(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
      glfwGetMouseButton(Window(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    model_transform_ = glm::rotate(glm::mat4{1.0f}, mouse_speed * float(diff_x),
                                   glm::vec3(0.0f, 1.0f, 0.0f)) *
                       model_transform_;
    model_transform_ = glm::rotate(glm::mat4{1.0f}, mouse_speed * float(diff_y),
                                   glm::vec3(1.0f, 0.0f, 0.0f)) *
                       model_transform_;
  } else if (glfwGetMouseButton(Window(), GLFW_MOUSE_BUTTON_LEFT) ==
             GLFW_PRESS) {
    camera_theta_ += -mouse_speed * diff_x;
    camera_phi_ += -mouse_speed * diff_y;
    camera_phi_ =
        glm::clamp(camera_phi_, -glm::half_pi<float>(), glm::half_pi<float>());
    camera_theta_ = glm::mod(camera_theta_, glm::two_pi<float>());
  } else if (glfwGetMouseButton(Window(), GLFW_MOUSE_BUTTON_RIGHT) ==
             GLFW_PRESS) {
    light_theta_ -= mouse_speed * diff_x;
    light_phi_ -= mouse_speed * diff_y;
    light_phi_ =
        glm::clamp(light_phi_, -glm::half_pi<float>(), glm::half_pi<float>());
    light_theta_ = glm::mod(light_theta_, glm::two_pi<float>());
  }

  glm::mat4 world =
      glm::translate(glm::mat4(1.0f), camera_position_) *
      glm::rotate(glm::mat4(1.0f), camera_theta_, glm::vec3(0.0f, 1.0f, 0.0f)) *
      glm::rotate(glm::mat4(1.0f), camera_phi_, glm::vec3(1.0f, 0.0f, 0.0f));
  const float camera_move_speed = 2.0f * duration_s;
  if (glfwGetKey(Window(), GLFW_KEY_A)) {
    camera_position_ -= camera_move_speed * glm::vec3(world[0]);
  }
  if (glfwGetKey(Window(), GLFW_KEY_D)) {
    camera_position_ += camera_move_speed * glm::vec3(world[0]);
  }
  if (glfwGetKey(Window(), GLFW_KEY_W)) {
    camera_position_ -= camera_move_speed * glm::vec3(world[2]);
  }
  if (glfwGetKey(Window(), GLFW_KEY_S)) {
    camera_position_ += camera_move_speed * glm::vec3(world[2]);
  }
  if (glfwGetKey(Window(), GLFW_KEY_F)) {
    camera_position_ -= camera_move_speed * glm::vec3(world[1]);
  }
  if (glfwGetKey(Window(), GLFW_KEY_R)) {
    camera_position_ += camera_move_speed * glm::vec3(world[1]);
  }

  glm::vec3 light_dir = glm::vec3(
      glm::cos(light_theta_) * glm::cos(light_phi_), glm::sin(light_phi_),
      glm::sin(light_theta_) * glm::cos(light_phi_));

  global_uniform_object_.proj =
      glm::perspective(glm::radians(45.0f),
                       static_cast<float>(Swapchain()->Extent().width) /
                           static_cast<float>(Swapchain()->Extent().height),
                       0.1f, 10.0f);
  global_uniform_object_.world = glm::inverse(world);
  global_uniform_object_.directional_light_direction =
      glm::normalize(glm::vec4(light_dir, 0.0f));
  global_uniform_object_.directional_light_color =
      glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
  global_uniform_object_.ambient_light_color =
      glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
  global_uniform_object_.specular_light = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);

  global_uniform_buffer_->At(0) = global_uniform_object_;

  EntityUniformObject entity_info = EntityUniformObject{
      model_transform_,
      glm::vec4{hsv2rgb(glm::vec3{light_h_, 0.7f, 1.0f}), 1.0f}};

  entity_->SetEntityInfo(entity_info);
  face_entity_->SetEntityInfo(entity_info);
}

void Lighting::OnRenderImpl(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    entity_pipeline_->Handle());

  VkDescriptorSet global_descriptor_set =
      global_descriptor_sets_[CurrentFrame()]->Handle();

  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          entity_pipeline_layout_->Handle(), 0, 1,
                          &global_descriptor_set, 0, nullptr);

  if (smoothed_model_) {
    entity_->Render(cmd_buffer, entity_pipeline_layout_->Handle());
  } else {
    face_entity_->Render(cmd_buffer, entity_pipeline_layout_->Handle());
  }
}

void Lighting::CreateGlobalAssets() {
  global_uniform_buffer_ =
      std::make_unique<DynamicBuffer<LightingGlobalUniformObject>>(this, 1);
  IgnoreResult(Device()->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT}},
      &global_descriptor_set_layout_));

  vulkan::DescriptorPoolSize pool_size =
      global_descriptor_set_layout_->GetPoolSize() * MaxFramesInFlight();
  IgnoreResult(Device()->CreateDescriptorPool(
      pool_size.ToVkDescriptorPoolSize(), MaxFramesInFlight(),
      &global_descriptor_pool_));

  global_descriptor_sets_.resize(MaxFramesInFlight());
  for (int i = 0; i < MaxFramesInFlight(); i++) {
    IgnoreResult(global_descriptor_pool_->AllocateDescriptorSet(
        global_descriptor_set_layout_->Handle(), &global_descriptor_sets_[i]));

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = global_uniform_buffer_->GetBuffer(i)->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(LightingGlobalUniformObject);

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    VkWriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = global_descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &buffer_info;

    write_descriptor_sets.push_back(write_descriptor_set);

    vkUpdateDescriptorSets(Device()->Handle(), write_descriptor_sets.size(),
                           write_descriptor_sets.data(), 0, nullptr);
  }
}

void Lighting::DestroyGlobalAssets() {
  global_descriptor_sets_.clear();
  global_descriptor_pool_.reset();
  global_descriptor_set_layout_.reset();
  global_uniform_buffer_.reset();
}

void Lighting::CreateEntityPipelineAssets() {
  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/lighting.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &entity_vert_shader_));

  IgnoreResult(Device()->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/lighting.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &entity_frag_shader_));

  IgnoreResult(
      Device()->CreatePipelineLayout({global_descriptor_set_layout_->Handle(),
                                      EntityDescriptorSetLayout()->Handle()},
                                     &entity_pipeline_layout_));

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

void Lighting::DestroyEntityPipelineAssets() {
  entity_pipeline_.reset();
  entity_pipeline_layout_.reset();
  entity_frag_shader_.reset();
  entity_vert_shader_.reset();
}

void Lighting::CreateEntities() {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;
  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        ASSETS_PATH "meshes/eight.uniform.obj")) {
    throw std::runtime_error(warn + err);
  }

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  for (const auto &shape : shapes) {
    vertices.resize(attrib.vertices.size() / 3);
    for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
      Vertex vertex{};
      vertex.pos = {attrib.vertices[i], attrib.vertices[i + 1],
                    attrib.vertices[i + 2]};
      vertex.normal = {0.0f, 0.0f, 0.0f};
      vertex.color = {1.0f, 1.0f, 1.0f};
      vertex.tex_coord = {0.5f, 0.5f};
      vertices[i / 3] = vertex;
    }
    indices.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
      indices[i] = shape.mesh.indices[i].vertex_index;
    }
  }

  std::vector<glm::vec3> normals;
  std::vector<float> weights;
  normals.resize(indices.size() / 3);
  weights.resize(indices.size() / 3);
  for (size_t i = 0; i < indices.size(); i += 3) {
    glm::vec3 v0 = vertices[indices[i]].pos;
    glm::vec3 v1 = vertices[indices[i + 1]].pos;
    glm::vec3 v2 = vertices[indices[i + 2]].pos;
    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    normals[i / 3] = normal;
    weights[i / 3] = glm::length(glm::cross(v1 - v0, v2 - v0));
  }
  std::vector<float> normal_accumulator(attrib.vertices.size(), 0.0f);
  for (size_t i = 0; i < indices.size(); ++i) {
    float weight = weights[i / 3];
    vertices[indices[i]].normal += normals[i / 3] * weight;
    normal_accumulator[indices[i]] += weight;
  }
  for (size_t i = 0; i < vertices.size(); ++i) {
    if (normal_accumulator[i] > 0.0f) {
      vertices[i].normal /= normal_accumulator[i];
    }
  }

  Image white_image;
  white_image(0, 0) = {255, 255, 255, 255};

  model_ = std::make_unique<Model>(this, vertices, indices);
  white_texture_ = std::make_unique<TextureImage>(this, white_image);
  entity_ = std::make_unique<Entity>(this, model_.get(), white_texture_.get());

  std::vector<Vertex> face_vertices;
  std::vector<uint32_t> face_indices;
  for (size_t i = 0; i < indices.size(); ++i) {
    Vertex vertex = vertices[indices[i]];
    vertex.normal = normals[i / 3];
    face_vertices.push_back(vertex);
    face_indices.push_back(i);
  }
  face_model_ = std::make_unique<Model>(this, face_vertices, face_indices);
  face_entity_ =
      std::make_unique<Entity>(this, face_model_.get(), white_texture_.get());
}

void Lighting::DestroyEntities() {
  face_entity_.reset();
  face_model_.reset();
  entity_.reset();
  white_texture_.reset();
  model_.reset();
}

void Lighting::OnKeyEvent(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_TAB:
        smoothed_model_ = !smoothed_model_;
        break;
      case GLFW_KEY_PAGE_UP:
        model_transform_ *= glm::scale(glm::mat4{1.0f}, glm::vec3{1.1f});
        break;
      case GLFW_KEY_PAGE_DOWN:
        model_transform_ *= glm::scale(glm::mat4{1.0f}, glm::vec3{1.0f / 1.1f});
        break;
    }
  }
}

void Lighting::OnScroll(double xoffset, double yoffset) {
  light_h_ += 0.05f * yoffset;
  light_h_ = glm::mod(light_h_, 1.0f);
}
