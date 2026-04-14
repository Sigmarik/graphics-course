#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iAlbedo;
layout(binding = 1) uniform sampler2D iAO;
layout(binding = 2) uniform sampler2D iDirectLight;

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

void main()
{
    vec2 uv = surf.wPos / 2.0 + vec2(0.5);   // texture coordinates
    vec3 albedo = texture(iAlbedo, uv).rgb;
    float ao = texture(iAO, uv).r;
    float direct = texture(iDirectLight, uv).r;
    out_fragColor.rgb = albedo * (ao * ao + direct);
}
