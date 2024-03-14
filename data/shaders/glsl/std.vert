#version 460 core
//#extension GL_GOOGLE_include_directive : require
//#include "shared.glsl"

layout(location=0) in vec3 in_pos;

layout(location=0) out vec3 out_color;

struct MaterialData
{
    vec3 color;
};

struct DrawData
{
    uint mat_id;
};

layout(set=0, binding=0) uniform FrameUBO
{
    mat4 proj_mat;
    mat4 view_mat;
} frame_ubo;;

layout(set=0, binding=1) buffer readonly MaterialSSBO
{
    MaterialData data[];
} mat_ssbo;

layout(set=0, binding=2) buffer readonly DrawSSBO
{
    DrawData data[];
} draw_ssbo;

void main()
{
    DrawData draw_data = draw_ssbo.data[gl_InstanceIndex];
    MaterialData mat_data = mat_ssbo.data[draw_data.mat_id];

    // gl_Position = frame_ubo.proj_mat * frame_ubo.view_mat * draw_data.model_matrix * vec4(in_pos, 1.0);
    // gl_Position = draw_data.model_matrix * vec4(in_pos, 1.0);
    gl_Position = vec4(in_pos, 1.0);

    out_color = mat_data.color;
    // out_color = vec3(1.0, 1.0, 0.0);
}