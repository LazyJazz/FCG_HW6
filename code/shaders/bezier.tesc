#version 450

layout (vertices = 4) out;

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

void main() {
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    if (gl_InvocationID == 0) {
        float tess_levels = GetFloat(75);
        gl_TessLevelInner[0] = tess_levels;
        gl_TessLevelInner[1] = tess_levels;
        gl_TessLevelOuter[0] = tess_levels;
        gl_TessLevelOuter[1] = tess_levels;
        gl_TessLevelOuter[2] = tess_levels;
        gl_TessLevelOuter[3] = tess_levels;
    }
}
