#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iAo;
layout(binding = 1) uniform sampler2D iDepth;
layout(binding = 2) uniform sampler2D iNormal;

layout(location = 0) out vec4 out_fragColor;

const int RADIUS = 4;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;   // texture coordinates
    vec2 texel_size = vec2(1.0) / textureSize(iDepth, 0).xy;

    vec3 center_normal = texture(iNormal, uv).rgb * 2.0 - 1.0;
    float center_depth = texture(iDepth, uv).r;
    float sigma_spatial = 1.5;
    float sigma_depth = 0.1;
    float exponent = 4;

    float ao_sum = 0.0;
    float weight_sum = 0.0;
    for (int i = -RADIUS; i <= RADIUS; ++i) {
        for (int j = -RADIUS; j <= RADIUS; ++j) {
            vec2 shifted = uv + vec2(i, j) * texel_size;
            float ao = texture(iAo, shifted).r;
            float depth = texture(iDepth, shifted).r;
            vec3 normal = texture(iNormal, shifted).rgb * 2.0 - 1.0;

            float spatial = exp(-(i*i + j*j) / (2.0 * sigma_spatial));
            float depth_w = exp(-abs(depth - center_depth) / sigma_depth);
            float normal_w = pow(max(0.0, dot(center_normal, normal)), exponent);

            float weight = spatial * depth_w * normal_w;
            ao_sum += ao * weight;
            weight_sum += weight;
        }
    }

    out_fragColor.r = ao_sum / weight_sum;
}