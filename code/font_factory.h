#pragma once
#include <ft2build.h>

#include "app.h"
#include "buffer.h"
#include "texture_image.h"
#include "utils.h"
#include FT_FREETYPE_H

struct FontModel {
  TextureImage *font_texture_{};
  vulkan::DescriptorSet *font_texture_descriptor_set_{};
  float bearing_x_{};
  float bearing_y_{};
  float advance_x_{};
};

struct FontInfo {
  glm::vec4 x_y_width_height;
  glm::vec4 color_depth;
};

struct FontDrawCalls {
  FontInfo font_info;
  vulkan::DescriptorSet *desc_set;
  bool operator<(const FontDrawCalls &other) const {
    return font_info.color_depth.w > other.font_info.color_depth.w;
  }
};

class FontFactory {
 public:
  FontFactory(Application *app);
  ~FontFactory();

  void SetFont(const std::string &font_path, uint32_t size);

  FontModel LoadChar(char c);

  void ClearDrawCalls();
  void CompileFontDrawCalls();

  void Render(VkCommandBuffer cmd_buffer);

  void DrawText(glm::vec2 pos,
                const std::string &text,
                const glm::vec3 &color,
                float depth);

  float GetTextWidth(const std::string &text);

 private:
  void CreateFontPipeline();
  void DestroyFontPipeline();

  Application *app_;

  FT_Library library_;
  std::map<std::string, FT_Face> loaded_faces_;
  std::map<std::pair<FT_Face, uint32_t>, std::map<char, FontModel>>
      loaded_fonts_;

  FT_Face activate_face_{nullptr};
  uint32_t activate_size_{32};
  std::map<char, FontModel> *activate_font_set_{nullptr};

  std::unique_ptr<vulkan::DescriptorSetLayout>
      font_global_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorSetLayout>
      font_image_descriptor_set_layout_;
  std::unique_ptr<vulkan::DescriptorPool> font_descriptor_pool_;
  std::vector<std::unique_ptr<vulkan::DescriptorSet>> font_descriptor_sets_;

  std::unique_ptr<vulkan::ShaderModule> font_vertex_shader_;
  std::unique_ptr<vulkan::ShaderModule> font_fragment_shader_;
  std::unique_ptr<vulkan::PipelineLayout> font_pipeline_layout_;
  std::unique_ptr<vulkan::Pipeline> font_pipeline_;

  std::vector<FontDrawCalls> font_infos_;
  std::unique_ptr<DynamicBuffer<glm::mat4>> global_transform_buffer_;
  std::unique_ptr<DynamicBuffer<FontInfo>> global_font_info_buffer_;
};
