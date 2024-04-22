#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 tex_coord;

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_color;
layout(location = 2) out vec2 frag_tex_coord;

layout(set = 0, binding = 0) uniform Camera {
  mat4 proj;
  mat4 world;
}
camera;

void main() {
  gl_Position = camera.proj * camera.world * vec4(position, 1.0);
  frag_normal = normalize(transpose(inverse(mat3(camera.world))) * normal);
  frag_color = color;
  frag_tex_coord = tex_coord;
}
