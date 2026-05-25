#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iAlbedo;
layout(binding = 1) uniform sampler2D iAO;
layout(binding = 2) uniform sampler2D iDirectLight;
layout(binding = 3) uniform sampler2D iFog;
layout(binding = 4) uniform sampler2D iSky;
layout(binding = 5) uniform sampler2D iDepth;
layout(binding = 6) uniform sampler2D iSkyBlurry;

layout(push_constant) uniform params_t
{
    mat4 invProjView;
    vec4 cameraPos;
} params;

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

vec3 transformSkyColor(vec3 color)
{
    float smoothing = 0.2;

    vec3 result;

    result.x = pow(color.x, smoothing);
    result.y = pow(color.y, smoothing);
    result.z = pow(color.z, smoothing);

    return result;
}

vec3 viewDir()
{
    vec4 pos = params.invProjView * vec4(surf.wPos, 0.5, 1.0);
    return normalize(pos.xyz / pos.w - params.cameraPos.xyz);
}

vec2 skyUvFromVector(vec3 dir)
{
    float theta = acos(clamp(dir.y, -1.0, 1.0));
    float phi = atan(dir.z, dir.x) + 3.1415 - 0.17;
    return vec2(phi / (2.0 * 3.14159265) + 0.5, theta / 3.14159265);
}

vec3 getSkyColor()
{
    return transformSkyColor(texture(iSky, skyUvFromVector(viewDir())).rgb);
}

void main()
{
    float brightnessBoost = 1.2;

    vec2 uv = surf.wPos / 2.0 + vec2(0.5);   // texture coordinates
    vec3 albedo = texture(iAlbedo, uv).rgb;
    float ao = texture(iAO, uv).r * brightnessBoost;
    float direct = texture(iDirectLight, uv).r * brightnessBoost;
    vec4 fog = texture(iFog, uv);
    float depth = texture(iDepth, uv).r;

    vec4 pos = params.invProjView * vec4(surf.wPos, depth, 1.0);
    vec3 dir = normalize(pos.xyz / pos.w - params.cameraPos.xyz);
    vec3 ambient = transformSkyColor(texture(iSkyBlurry, skyUvFromVector(dir)).rgb);

    vec3 ambientView = transformSkyColor(texture(iSkyBlurry, skyUvFromVector(viewDir())).rgb);

    vec3 rawColor = albedo * (ao * ao * ambient + vec3(direct));
//    rawColor *= fog.a;

    if (depth == 1.0)
    {
        rawColor = getSkyColor();
    }

    rawColor += fog.xyz * ambientView;
    out_fragColor = vec4(rawColor, 1.0);
}
