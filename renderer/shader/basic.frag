#version 450 core

#extension GL_EXT_scalar_block_layout : require

layout(location = 0) in vec3 frag_uv;
layout(location = 1) in vec4 frag_colour;

layout(location = 0) out vec4 framebuffer;

layout(binding = 1) uniform sampler u_sampler;
layout(binding = 2) uniform texture2DArray u_texture;

void main() {
    framebuffer = frag_colour * texture(sampler2DArray(u_texture, u_sampler), frag_uv);
}
