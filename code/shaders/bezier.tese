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

vec3 BezierInterpolationLinear(int ix, float v) {
    vec3 p[5];
    for (int i = 0; i < 5; i++) {
        p[i] = GetVec3(ix, i);
    }
    for (int i = 1; i < 5; i++) {
        for (int j = 0; j < 5 - i; j++) {
            p[j] = mix(p[j], p[j + 1], v);
        }
    }
    return p[0];
}

vec3 BezierInterpolation(float u, float v) {
    vec3 p[5];
    for (int i = 0; i < 5; i++) {
        p[i] = BezierInterpolationLinear(i, v);
    }
    for (int i = 1; i < 5; i++) {
        for (int j = 0; j < 5 - i; j++) {
            p[j] = mix(p[j], p[j + 1], u);
        }
    }
    return p[0];
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
