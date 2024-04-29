#version 450

layout(location = 0) in vec2 pos;
layout(location = 1) in float size;
layout(location = 2) in vec3 color;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;

vec2 positions[6] = vec2[6](vec2(-0.5, -0.5),
                            vec2(0.5, 0.5),
                            vec2(-0.5, 0.5),
                            vec2(-0.5, -0.5),
                            vec2(0.5, 0.5),
                            vec2(0.5, -0.5));

layout(binding = 0) uniform GlobalUniformObject {
  mat4 transform;
};

void main() {
  vec2 vert_pos = positions[gl_VertexIndex];
  gl_Position = (transform * vec4(vert_pos * size + pos, 0.0, 1.0)) *
                vec4(1.0, -1.0, 1.0, 1.0);
  frag_color = color;
  frag_tex_coord = vert_pos + 0.5;
}
