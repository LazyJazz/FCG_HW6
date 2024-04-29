#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D tex;

void main() {
  float alpha = texture(tex, tex_coord).r;
  out_color = vec4(color * alpha, alpha);
}
