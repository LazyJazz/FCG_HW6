#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec4 out_color;

layout(binding = 2) uniform sampler2D tex;

void main() {
  out_color = texture(tex, tex_coord) * color;
}
