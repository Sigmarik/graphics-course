#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

layout(location = 0) out vec4 out_fragColor;

//layout(push_constant) uniform params_t
//{
//    vec3 Color;
//} params;

layout(std430, binding = 0) readonly buffer bind0in
{
    vec3 color;
};

void main() {
    out_fragColor = vec4(color, 1.0);
}
