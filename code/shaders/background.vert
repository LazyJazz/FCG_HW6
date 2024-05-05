#version 450

vec2 pos[6] = vec2[6](vec2(-1.0, -1.0),
                      vec2(1.0, -1.0),
                      vec2(-1.0, 1.0),
                      vec2(1.0, 1.0),
                      vec2(-1.0, 1.0),
                      vec2(1.0, -1.0));

layout(location = 0) out vec2 frag_tex_coord;

void main() {
  gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
  frag_tex_coord = pos[gl_VertexIndex] * 0.5 + 0.5;
}
