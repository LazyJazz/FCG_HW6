#include "solar_system.h"

#include "celestial_body.h"

namespace {
#include "built_in_shaders.inl"
}

void SolarSystem::OnInitImpl() {
  CreateGlobalAssets();
  CreateEntityPipelineAssets();
  CreateEntities();
  CreateFontFactory();
  CreateCelestialBodies();
}

void SolarSystem::OnShutdownImpl() {
  DestroyCelestialBodies();
  DestroyFontFactory();
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
  static auto last_time = std::chrono::steady_clock::now();
  auto current_time = std::chrono::steady_clock::now();
  float delta_t =
      std::chrono::duration<float>(current_time - last_time).count();
  last_time = current_time;

  font_factory_->ClearDrawCalls();
  auto extent = Swapchain()->Extent();
  float aspect = extent.width / static_cast<float>(extent.height);
  global_uniform_object_.proj =
      glm::perspective(glm::radians(45.0f), aspect, 0.1f, 40.0f);

  float camera_angular_speed = glm::radians(90.0f);
  if (glfwGetKey(Window(), GLFW_KEY_A)) {
    camera_theta_ += camera_angular_speed * delta_t;
  }
  if (glfwGetKey(Window(), GLFW_KEY_D)) {
    camera_theta_ -= camera_angular_speed * delta_t;
  }

  float sin_camera_theta = std::sin(camera_theta_);
  float cos_camera_theta = std::cos(camera_theta_);

  global_uniform_object_.world = glm::lookAt(
      glm::vec3{sin_camera_theta * 15.0f, 0.0f, cos_camera_theta * 15.0f},
      glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
  global_uniform_buffer_->At(0) = global_uniform_object_;

  global_t_ += delta_t * time_flowing_ratio_;

  for (auto planet : planets_) {
    planet->Update(global_t_);
  }

  if (show_planet_name_) {
    font_factory_->SetFont(font_types_[font_select_], 32);

    glm::vec3 planet_name_color{0.6f, 0.7f, 0.8f};

    for (auto planet : planets_) {
      glm::vec4 pos = global_uniform_object_.proj *
                      global_uniform_object_.world *
                      glm::vec4{planet->GetPosition(), 1.0f};
      double scaled_radius = planet->GetRadius() / pos.w;
      pos /= pos.w;
      if (pos.z < 0.0f || pos.z > 1.0f) {
        continue;
      }
      pos.x = (pos.x + 1.0f) * 0.5f * extent.width;
      pos.y = (1.0f - pos.y) * 0.5f * extent.height;
      pos.y -= scaled_radius * extent.height * 1.0f;
      pos.y -= 30.0f;
      pos.x -= font_factory_->GetTextWidth(planet->GetName()) * 0.5f;
      font_factory_->DrawText(glm::vec2{pos.x, pos.y}, planet->GetName(),
                              planet_name_color, pos.z);
    }
  }

  if (show_usage_info_) {
    font_factory_->SetFont(ASSETS_PATH "font/consola.ttf", 16);
    font_factory_->DrawText(glm::vec2{10.0f, extent.height - 14.0f},
                            "Press ENTER to turn on/off usage info.",
                            glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
    font_factory_->DrawText(glm::vec2{10.0f, extent.height - 32.0f},
                            "Press F to toggle celestial body name font.",
                            glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
    font_factory_->DrawText(glm::vec2{10.0f, extent.height - 50.0f},
                            "Press Ctrl to turn on/off celestial body name.",
                            glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
    font_factory_->DrawText(
        glm::vec2{10.0f, extent.height - 68.0f},
        fmt::format("Current speed is: {:.1f}", time_flowing_ratio_),
        glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
    font_factory_->DrawText(glm::vec2{10.0f, extent.height - 86.0f},
                            "Use LEFT/RIGHT to speed down/up.",
                            glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
    font_factory_->DrawText(glm::vec2{10.0f, extent.height - 104.0f},
                            "Use A/D to rotate camera.",
                            glm::vec3{1.0f, 1.0f, 1.0f}, 0.0f);
  }

  font_factory_->CompileFontDrawCalls();
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

  font_factory_->Render(cmd_buffer);
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
    for (int j = 0; j < precision + 1; j++) {
      float theta = 2.0f * glm::pi<float>() * float(j) * inv_precision;
      glm::vec3 pos = glm::vec3{glm::cos(theta) * glm::sin(phi), glm::cos(phi),
                                -glm::sin(theta) * glm::sin(phi)};
      glm::vec3 normal = glm::normalize(pos);
      glm::vec3 color = glm::vec3{1.0f, 1.0f, 1.0f};
      glm::vec2 tex_coord =
          glm::vec2{float(j) * inv_precision, float(i) * inv_precision};
      sphere_vertices.push_back({pos, normal, color, tex_coord});

      if (i && j) {
        int i1 = i - 1;
        int j1 = j - 1;
        sphere_indices.push_back(i1 * (precision + 1) + j);
        sphere_indices.push_back(i1 * (precision + 1) + j1);
        sphere_indices.push_back(i * (precision + 1) + j);
        sphere_indices.push_back(i1 * (precision + 1) + j1);
        sphere_indices.push_back(i * (precision + 1) + j1);
        sphere_indices.push_back(i * (precision + 1) + j);
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

void SolarSystem::CreateFontFactory() {
  font_factory_ = std::make_unique<FontFactory>(this);
  font_factory_->SetFont(ASSETS_PATH "font/georgia.ttf", 32);
}

void SolarSystem::DestroyFontFactory() {
  font_factory_.reset();
}

void SolarSystem::CreateCelestialBodies() {
  // clang-format off
  sun_ = std::make_unique<CelestialBody>(this, nullptr, 1.2f, 0.0f, 365.0f / 25.0f, 0.0f, 0.0f, 0.0f, ASSETS_PATH "texture/sun.jpg", "Sun");
  mercury_ = std::make_unique<CelestialBody>(this, nullptr, 0.05f, 1.35f, 1.0f / 58.6462f, 365.2564f / 87.9674f, 0.0f, 100.0f, ASSETS_PATH "texture/mercury.jpg", "Mercury");
  venus_ = std::make_unique<CelestialBody>(this, nullptr, 0.16f, 1.6f, 1.0f / 243.0187f, 365.2564f / 224.6960f, 0.0f, 200.0f, ASSETS_PATH "texture/venus.jpg", "Venus");
  earth_ = std::make_unique<CelestialBody>(this, nullptr, 0.18f, 2.0f, 1.0f, 365.2564f / 365.2564f, 0.0f, 300.0f, ASSETS_PATH "texture/earth.jpg", "Earth");
  mars_ = std::make_unique<CelestialBody>(this, nullptr, 0.1f, 2.4f, 23.9345f / 24.6230, 365.2564f / 686.9649, 0.0f, 400.0f, ASSETS_PATH "texture/mars.jpg", "Mars");
  jupiter_ = std::make_unique<CelestialBody>(this, nullptr, 0.8f, 3.4f, 23.9345f / 9.9250f, 3.0f / 11.862615, 0.0f, 800.0f, ASSETS_PATH "texture/jupiter.jpg", "Jupiter");
  saturn_ = std::make_unique<CelestialBody>(this, nullptr, 0.7f, 5.2f, 23.9345f / 10.6562f, 3.0f / 29.447498, 0.0f, 1600.0f, ASSETS_PATH "texture/saturn.jpg", "Saturn");
  uranus_ = std::make_unique<CelestialBody>(this, nullptr, 0.6f, 6.7f, 23.9345f / 17.2399f, 3.0f / 84.016846, 0.0f, 2200.0f, ASSETS_PATH "texture/uranus.jpg", "Uranus");
  neptune_ = std::make_unique<CelestialBody>(this, nullptr, 0.55f, 8.2f, 23.9345f / 16.1100f, 3.0f / 164.79132, 0.0f, 3000.0f, ASSETS_PATH "texture/neptune.jpg", "Neptune");
  moon_ = std::make_unique<CelestialBody>(this, earth_.get(), 0.03f, 0.2f, -12.0f, 12.0f, 0.0f, 0.0f, ASSETS_PATH "texture/moon.jpg", "Moon");
  // clang-format on

  planets_.push_back(sun_.get());
  planets_.push_back(mercury_.get());
  planets_.push_back(venus_.get());
  planets_.push_back(earth_.get());
  planets_.push_back(mars_.get());
  planets_.push_back(jupiter_.get());
  planets_.push_back(saturn_.get());
  planets_.push_back(uranus_.get());
  planets_.push_back(neptune_.get());
  planets_.push_back(moon_.get());
}

void SolarSystem::DestroyCelestialBodies() {
  moon_.reset();
  neptune_.reset();
  uranus_.reset();
  saturn_.reset();
  jupiter_.reset();
  mars_.reset();
  earth_.reset();
  venus_.reset();
  mercury_.reset();
  sun_.reset();
}

SolarSystem::SolarSystem() {
  glfwSetWindowUserPointer(Window(), this);
  glfwSetKeyCallback(Window(), [](GLFWwindow *window, int key, int scancode,
                                  int action, int mods) {
    auto app =
        reinterpret_cast<SolarSystem *>(glfwGetWindowUserPointer(window));
    app->KeyCallBack(key, scancode, action, mods);
  });

  font_types_[0] = ASSETS_PATH "font/consola.ttf";
  font_types_[1] = ASSETS_PATH "font/georgia.ttf";
}

void SolarSystem::KeyCallBack(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS || action == GLFW_REPEAT) {
    if (key == GLFW_KEY_F) {
      font_select_ = (font_select_ + 1) % 2;
    } else if (key == GLFW_KEY_ENTER) {
      show_usage_info_ = !show_usage_info_;
    } else if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
      show_planet_name_ = !show_planet_name_;
    } else if (key == GLFW_KEY_LEFT) {
      time_flowing_ratio_ -= 0.1f;
    } else if (key == GLFW_KEY_RIGHT) {
      time_flowing_ratio_ += 0.1f;
    }
    if (std::fabs(time_flowing_ratio_) < 1e-5f) {
      time_flowing_ratio_ = 0.0f;
    }
  }
}
