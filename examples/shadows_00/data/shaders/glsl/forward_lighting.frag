#version 460 core

layout(location=0) in vec3 in_color;

layout(location=0) out vec4 out_color;

struct MaterialData
{
    vec3 color;
    uint __padding;
};

struct DrawData
{
    mat4 model_matrix;
    uint mat_id;
    uint __padding[3];
};

#include "frame_desc_bindings.glsl"

void main()
{
    out_color = vec4(in_color, 1.0f);
}