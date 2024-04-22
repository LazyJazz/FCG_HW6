#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_color;
layout(location = 2) out vec2 frag_tex_coord;

layout(set = 0, binding = 0) uniform GlobalUniformObject {
  mat4 proj;
  mat4 world;
}
camera;

layout(set = 1, binding = 1) uniform EntityUniformObject {
  mat4 model;
}
entity;

void main() {
  gl_Position =
      (camera.proj * camera.world * entity.model * vec4(position, 1.0)) *
      vec4(1.0, -1.0, 1.0, 1.0);
  frag_normal = normalize(transpose(inverse(mat3(entity.model))) * normal);
  frag_color = color;
  frag_tex_coord = tex_coord;
}
