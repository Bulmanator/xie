#version 450 core

#extension GL_EXT_scalar_block_layout : require

struct Vertex3 {
    vec3 p;
    vec3 uv;
    uint c;
};

layout(push_constant, scalar, row_major)
uniform R_Setup {
    mat4  view_proj;
    vec4  camera_p;
    float time;
    float dt;
    uvec2 window_dim;
} setup;

layout(binding = 0, scalar)
readonly buffer Vertices {
    Vertex3 vertices[];
};

layout(location = 0) out vec3 frag_uv;
layout(location = 1) out vec4 frag_colour;

void main() {
    Vertex3 vtx = vertices[gl_VertexIndex];

    gl_Position = setup.view_proj * vec4(vtx.p, 1.0);

    frag_uv     = vtx.uv;
    frag_colour = unpackUnorm4x8(vtx.c); // might need swizzling!
}
