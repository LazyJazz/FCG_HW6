#include "font_factory.h"

namespace {
#include "built_in_shaders.inl"
}

FontFactory::FontFactory(Application *app) : app_(app) {
  auto device = app_->Device();
  FT_Init_FreeType(&library_);
  CreateFontPipeline();
}

FontFactory::~FontFactory() {
  for (auto &font : loaded_fonts_) {
    for (auto &font_model : font.second) {
      delete font_model.second.font_texture_;
      delete font_model.second.font_texture_descriptor_set_;
    }
  }

  for (auto &face : loaded_faces_) {
    FT_Done_Face(face.second);
  }

  FT_Done_FreeType(library_);
  DestroyFontPipeline();
}

void FontFactory::SetFont(const std::string &font_path, uint32_t size) {
  if (!loaded_faces_.count(font_path)) {
    FT_Face face;
    FT_New_Face(library_, font_path.c_str(), 0, &face);
    loaded_faces_[font_path] = face;
  }
  activate_face_ = loaded_faces_[font_path];
  activate_size_ = size;
  if (!loaded_fonts_.count({activate_face_, activate_size_})) {
    loaded_fonts_[{activate_face_, activate_size_}] = {};
  }
  activate_font_set_ = &loaded_fonts_[{activate_face_, activate_size_}];
}

FontModel FontFactory::LoadChar(char c) {
  if (!activate_font_set_->count(c)) {
    FT_Set_Pixel_Sizes(activate_face_, 0, activate_size_);
    FT_Load_Char(activate_face_, c, FT_LOAD_RENDER);
    auto glyph = activate_face_->glyph;
    auto bitmap = glyph->bitmap;
    auto width = bitmap.width;
    auto height = bitmap.rows;
    auto image = Image(width, height, bitmap.buffer);
    TextureImage *texture_image = nullptr;
    vulkan::DescriptorSet *descriptor_set{nullptr};

    if (width && height) {
      texture_image = new TextureImage(app_, image);
      font_descriptor_pool_->AllocateDescriptorSet(
          font_image_descriptor_set_layout_->Handle(), &descriptor_set);

      VkDescriptorImageInfo image_info{};
      image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      image_info.imageView = texture_image->GetImage()->ImageView();
      image_info.sampler = app_->EntitySampler()->Handle();

      VkWriteDescriptorSet write_descriptor_set{};
      write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor_set.dstSet = descriptor_set->Handle();
      write_descriptor_set.dstBinding = 0;
      write_descriptor_set.dstArrayElement = 0;
      write_descriptor_set.descriptorType =
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write_descriptor_set.descriptorCount = 1;
      write_descriptor_set.pImageInfo = &image_info;

      vkUpdateDescriptorSets(app_->Device()->Handle(), 1, &write_descriptor_set,
                             0, nullptr);
    }

    auto font_model = FontModel{texture_image, descriptor_set,
                                float(glyph->metrics.horiBearingX) / 64.0f,
                                float(glyph->metrics.horiBearingY) / 64.0f,
                                float(glyph->metrics.horiAdvance) / 64.0f};
    activate_font_set_->insert({c, font_model});
  }
  return activate_font_set_->at(c);
}

void FontFactory::CreateFontPipeline() {
  auto device = app_->Device();
  IgnoreResult(device->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/font.vert"),
                                 VK_SHADER_STAGE_VERTEX_BIT),
      &font_vertex_shader_));
  IgnoreResult(device->CreateShaderModule(
      vulkan::CompileGLSLToSPIRV(GetShaderCode("shaders/font.frag"),
                                 VK_SHADER_STAGE_FRAGMENT_BIT),
      &font_fragment_shader_));

  IgnoreResult(device->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT},
       {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}},
      &font_global_descriptor_set_layout_));

  IgnoreResult(device->CreateDescriptorSetLayout(
      {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
        VK_SHADER_STAGE_FRAGMENT_BIT}},
      &font_image_descriptor_set_layout_));

  vulkan::DescriptorPoolSize pool_size =
      (font_global_descriptor_set_layout_->GetPoolSize() *
       app_->MaxFramesInFlight()) +
      (font_image_descriptor_set_layout_->GetPoolSize() * 2048);
  IgnoreResult(device->CreateDescriptorPool(pool_size.ToVkDescriptorPoolSize(),
                                            app_->MaxFramesInFlight() + 2048,
                                            &font_descriptor_pool_));
  global_transform_buffer_ =
      std::make_unique<DynamicBuffer<glm::mat4>>(app_, 1);
  global_font_info_buffer_ =
      std::make_unique<DynamicBuffer<FontInfo>>(app_, 16384);

  font_descriptor_sets_.resize(app_->MaxFramesInFlight());
  for (int i = 0; i < app_->MaxFramesInFlight(); i++) {
    font_descriptor_pool_->AllocateDescriptorSet(
        font_global_descriptor_set_layout_->Handle(),
        &font_descriptor_sets_[i]);

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = global_transform_buffer_->GetBuffer(i)->Handle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(glm::mat4);

    VkDescriptorBufferInfo buffer_info2{};
    buffer_info2.buffer = global_font_info_buffer_->GetBuffer(i)->Handle();
    buffer_info2.offset = 0;
    buffer_info2.range = sizeof(FontInfo) * global_font_info_buffer_->Size();

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    VkWriteDescriptorSet write_descriptor_set{};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = font_descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &buffer_info;
    write_descriptor_sets.push_back(write_descriptor_set);

    write_descriptor_set = {};
    write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.dstSet = font_descriptor_sets_[i]->Handle();
    write_descriptor_set.dstBinding = 1;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &buffer_info2;
    write_descriptor_sets.push_back(write_descriptor_set);

    vkUpdateDescriptorSets(device->Handle(), write_descriptor_sets.size(),
                           write_descriptor_sets.data(), 0, nullptr);
  }

  IgnoreResult(device->CreatePipelineLayout(
      {
          font_global_descriptor_set_layout_->Handle(),
          font_image_descriptor_set_layout_->Handle(),
      },
      &font_pipeline_layout_));

  vulkan::PipelineSettings settings{app_->RenderPass(),
                                    font_pipeline_layout_.get(), 0};
  settings.AddShaderStage(font_vertex_shader_.get(),
                          VK_SHADER_STAGE_VERTEX_BIT);
  settings.AddShaderStage(font_fragment_shader_.get(),
                          VK_SHADER_STAGE_FRAGMENT_BIT);
  settings.SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  settings.SetBlendState(
      0, VkPipelineColorBlendAttachmentState{
             VK_TRUE,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
             VK_BLEND_OP_ADD,
             VK_BLEND_FACTOR_ONE,
             VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
             VK_BLEND_OP_ADD,
             VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
         });
  settings.SetCullMode(VK_CULL_MODE_NONE);
  settings.SetMultiSampleState(VK_SAMPLE_COUNT_1_BIT);

  IgnoreResult(device->CreatePipeline(settings, &font_pipeline_));
}

void FontFactory::DestroyFontPipeline() {
  font_pipeline_.reset();
  font_pipeline_layout_.reset();

  font_descriptor_sets_.clear();

  global_transform_buffer_.reset();
  global_font_info_buffer_.reset();

  font_descriptor_pool_.reset();
  font_image_descriptor_set_layout_.reset();
  font_global_descriptor_set_layout_.reset();

  font_vertex_shader_.reset();
  font_fragment_shader_.reset();
}

void FontFactory::ClearDrawCalls() {
  font_infos_.clear();
}

void FontFactory::CompileFontDrawCalls() {
  std::sort(font_infos_.begin(), font_infos_.end());
  FontInfo *font_info_data = global_font_info_buffer_->Data();
  for (int i = 0; i < font_infos_.size(); i++) {
    font_info_data[i] = font_infos_[i].font_info;
  }
  VkExtent2D extent = app_->Swapchain()->Extent();
  // clang-format off
  global_transform_buffer_->At(0) = glm::mat4{
      2.0f / float(extent.width), 0.0f, 0.0f, 0.0f,
      0.0f, 2.0f / float(extent.height), 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      -1.0f, -1.0f, 0.0f, 1.0f};
  // clang-format on
}

void FontFactory::Render(VkCommandBuffer cmd_buffer) {
  vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    font_pipeline_->Handle());
  VkDescriptorSet descriptor_sets[] = {
      font_descriptor_sets_[app_->CurrentFrame()]->Handle(),
  };
  vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          font_pipeline_layout_->Handle(), 0, 1,
                          descriptor_sets, 0, nullptr);
  for (size_t i = 0; i < font_infos_.size(); i++) {
    auto font_info = font_infos_[i];
    if (font_info.desc_set) {
      descriptor_sets[0] = font_info.desc_set->Handle();
      vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              font_pipeline_layout_->Handle(), 1, 1,
                              descriptor_sets, 0, nullptr);
      vkCmdDraw(cmd_buffer, 6, 1, 0, i);
    }
  }
}

void FontFactory::DrawText(glm::vec2 pos,
                           const std::string &text,
                           const glm::vec3 &color,
                           float depth) {
  for (auto c : text) {
    auto font = LoadChar(c);
    VkExtent2D extent{};
    if (font.font_texture_) {
      extent = font.font_texture_->GetImage()->Extent();
    }
    font_infos_.push_back({{{pos.x + font.bearing_x_, pos.y - font.bearing_y_,
                             extent.width, extent.height},
                            {color.r, color.g, color.b, depth}},
                           font.font_texture_descriptor_set_});
    pos.x += font.advance_x_;
  }
}
