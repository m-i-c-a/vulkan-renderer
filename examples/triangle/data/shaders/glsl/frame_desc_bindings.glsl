struct ForwardPointLightData
{
    vec3 position;
    vec3 attenuation_coefs; // constant, linear, quadratic
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

layout(set=0, binding=0) uniform Frame_UBO
{
    mat4 proj_mat;
    mat4 view_mat;

    vec3 dir_light_direction;
    vec3 dir_light_ambient;
    vec3 dir_light_diffuse;
    vec3 dir_light_specular;

} frame_ubo;

layout(set=0, binding=1) buffer readonly Frame_MaterialSSBO
{
    MaterialData data[];
} frame_mat_ssbo;

layout(set=0, binding=2) buffer readonly Frame_DrawSSBO
{
    DrawData data[];
} frame_draw_ssbo;

layout(set=0, binding=3) uniform Frame_ForwardPointLightUBO
{
    uint num_point_lights;
    ForwardPointLightData point_light_list[32];
} frame_forward_light_ubo;