#version 450

layout(set = 0, binding = 0) uniform GlobalTransform {
  mat4 global_transform;
};

struct FontInfo {
  vec4 x_y_width_height;
  vec4 color_depth;
};

layout(set = 0, binding = 1) buffer InstanceData {
  FontInfo font_infos[];
};

vec2 vertices[6] = vec2[](vec2(0.0, 0.0),
                          vec2(0.0, 1.0),
                          vec2(1.0, 0.0),
                          vec2(0.0, 1.0),
                          vec2(1.0, 0.0),
                          vec2(1.0, 1.0));

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec3 frag_color;

void main() {
  vec2 pos2 = vertices[gl_VertexIndex];
  FontInfo font_info = font_infos[gl_InstanceIndex];
  vec4 pos =
      vec4(pos2 * font_info.x_y_width_height.zw + font_info.x_y_width_height.xy,
           font_info.color_depth.w, 1.0);
  gl_Position = global_transform * pos;
  frag_tex_coord = pos2;
  frag_color = font_info.color_depth.xyz;
}
