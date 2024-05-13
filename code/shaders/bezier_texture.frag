#version 450

layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D tex;

void main() {
  out_color = texture(tex, tex_coord);
}
