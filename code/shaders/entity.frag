#version 450

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_color;
layout(location = 2) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
  out_color = vec4(frag_color, 1.0) * texture(tex, frag_tex_coord);
}
