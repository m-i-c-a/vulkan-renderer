#version 460 core
#extension GL_ARB_draw_instanced : enable

layout(location=0) in vec3 in_pos;

layout(location=0) out vec3 out_color;

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
    DrawData draw_data = frame_draw_ssbo.data[gl_InstanceIndex];
    MaterialData mat_data = frame_mat_ssbo.data[draw_data.mat_id];

    gl_Position = frame_ubo.proj_mat * frame_ubo.view_mat * draw_data.model_matrix * vec4(in_pos, 1.0);
    out_color = mat_data.color;
}