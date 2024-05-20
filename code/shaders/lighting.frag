#version 450

layout(location = 0) in vec3 frag_normal;
layout(location = 1) in vec3 frag_color;
layout(location = 2) in vec2 frag_tex_coord;
layout(location = 3) in vec3 frag_pos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform GlobalUniformObject {
  mat4 proj;
  mat4 world;
  vec4 directional_light_direction;
  vec4 directional_light_color;
  vec4 specular_color;
  vec4 ambient_light_color;
}
camera;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(set = 1, binding = 1) uniform EntityUniformObject {
  mat4 model;
  vec4 color;
}
entity;

void main() {
  vec4 light_strenth = vec4(0.0);
  light_strenth +=
      max(dot(normalize(frag_normal), camera.directional_light_direction.xyz),
          0.0) *
      camera.directional_light_color;
  light_strenth += camera.ambient_light_color;
  light_strenth.a = 1.0;
  out_color = entity.color * vec4(frag_color, 1.0) *
              texture(tex, frag_tex_coord) * light_strenth;
  vec3 view_dir = normalize(inverse(camera.world)[3].xyz - frag_pos);
  vec3 reflect_dir =
      reflect(-camera.directional_light_direction.xyz, normalize(frag_normal));
  out_color +=
      pow(max(dot(view_dir, reflect_dir), 0.0), 32) * camera.specular_color;
  out_color.a = 1.0;
}
