#version 450

layout (quads, equal_spacing, ccw) in;

layout (location = 0) out vec2 tex_coord;

layout (binding = 0, std140) uniform GlobalUniformBuffer {
    mat4 proj;
    mat4 view;
    vec4 vectorized_control_point_positions[19];
};

float GetFloat(int i) {
    return vectorized_control_point_positions[i >> 2][i & 3];
}

vec3 GetVec3(int ix, int iy) {
    int i = (ix * 5 + iy) * 3;
    return vec3(GetFloat(i), GetFloat(i + 1), GetFloat(i + 2));
}

float factorial[5] = float[5](1.0, 1.0, 2.0, 6.0, 24.0);

vec3 BezierInterpolationLinear(int ix, float v) {
    vec3 p = vec3(0.0);
    for (int i = 0; i < 5; i++) {
        p += GetVec3(ix, i) * factorial[4] / (factorial[i] * factorial[4 - i]) * pow(v, float(i)) * pow(1.0 - v, float(4 - i));
    }
    return p;
}

vec3 BezierInterpolation(float u, float v) {
    vec3 p = vec3(0.0);
    for (int i = 0; i < 5; i++) {
        p += BezierInterpolationLinear(i, v) * factorial[4] / (factorial[i] * factorial[4 - i]) * pow(u, float(i)) * pow(1.0 - u, float(4 - i));
    }
    return p;
}

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    vec3 pos = BezierInterpolation(u, v);
//    = vec3(mix(
//    mix(gl_in[0].gl_Position, gl_in[1].gl_Position, u),
//    mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u),
//    v
//    ));
    tex_coord = vec2(u, v);
    gl_Position = proj * view * vec4(pos, 1.0);
}
