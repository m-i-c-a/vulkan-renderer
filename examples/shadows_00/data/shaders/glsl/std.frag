#version 460 core

layout(location=0) in vec3 in_color;

layout(location=0) out vec4 out_color;

layout(set=1, binding=0) uniform sampler2D Pass_InputAttachments[1];

void main()
{
    out_color = vec4(vec3(texelFetch(Pass_InputAttachments[0], ivec2(gl_FragCoord.xy), 0).r), 1.0f);
    // out_color = vec4(in_color, 1.0f);
}
