#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iDirect;
layout(binding = 1) uniform sampler2D iDepth;
layout(binding = 2) uniform sampler2D iNormal;

layout(location = 0) out vec4 out_fragColor;

layout(push_constant) uniform PushConstants
{
    mat4 proj;
    mat4 invProj;
} pc;

const uint SAMPLE_COUNT = 16;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

float random(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;   // texture coordinates
    vec2 texel_size = vec2(1.0) / textureSize(iDepth, 0).xy;

    vec2 ndc = surf.wPos;
    float depthVal = texture(iDepth, uv).r;
    vec4 clipPos = vec4(ndc, depthVal * 2.0 - 1.0, 1.0);
    vec4 viewPos4 = pc.invProj * clipPos;
    vec3 viewPos = viewPos4.xyz / viewPos4.w;
    float trueDepth = length(viewPos);

    vec3 center_normal = texture(iNormal, uv).rgb * 2.0 - 1.0;
    float center_depth = texture(iDepth, uv).r;
    float sigma_depth = 0.5;
    float exponent = 2.0;

    float angle = random(uv) * 6.28318530718;
    float ca = cos(angle);
    float sa = sin(angle);
    mat2 rot = mat2(ca, sa, -sa, ca);

    float direct_sum = 0.0;
    float weight_sum = 0.0;
    float worldRadius = 0.1;
    float radius_uv = worldRadius / trueDepth;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float r = sqrt((float(i) + 0.5) / float(SAMPLE_COUNT));
        float theta = float(i) * 2.39996323; // Golden angle
        vec2 offset = vec2(cos(theta), sin(theta)) * r;
        offset = rot * offset;

        vec2 shifted = uv + offset * radius_uv;
        float direct = texture(iDirect, shifted).r;
        float depth = texture(iDepth, shifted).r;
        vec3 normal = texture(iNormal, shifted).rgb * 2.0 - 1.0;

        // Decrease weight radially
        float spatial = exp(-(r * r) * 4.0);
        // Decrease weight if depth difference is too high (relative depth usually makes more sense, but ok)
        // Adjust depth_w to not be too narrow
        float depth_w = exp(-abs(depth - center_depth) / sigma_depth);
        // Falloff on sharp normal angles
        float normal_w = pow(max(0.0, dot(center_normal, normal)), exponent);

        float weight = spatial * depth_w * normal_w;
        direct_sum += direct * weight;
        weight_sum += weight;
    }

    out_fragColor.r = direct_sum / weight_sum;
}