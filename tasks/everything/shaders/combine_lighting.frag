#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iAlbedo;
layout(binding = 1) uniform sampler2D iAO;

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

void main()
{
    vec3 albedo = texture(iAlbedo, surf.wPos / 2.0 + vec2(0.5)).rgb;
    float ao = texture(iAO, surf.wPos / 2.0 + vec2(0.5)).r;
    out_fragColor.rgb = albedo * ao;
}
