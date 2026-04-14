#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iDepth;
layout(binding = 1) uniform sampler2D iNormal;
layout(binding = 2) uniform sampler2D iShadowmap;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

layout(location = 0) out vec4 out_fragColor;

layout(push_constant) uniform PushConstants
{
    mat4 invProjView;
    mat4 lightProjView;
    mat4 invView;
    mat4 lightInvView;
} pc;

vec3 world_position()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;

    vec2 ndc = surf.wPos;
    float depthVal = texture(iDepth, uv).r;
    vec4 clipPos = vec4(ndc, depthVal, 1.0);
    vec4 worldPos4 = pc.invProjView * clipPos;
    return worldPos4.xyz / worldPos4.w;
}

vec2 spread_clip_pos(vec2 position)
{
//    float len = length(position);
//    float betterLength = sqrt(len);
//    return position * (betterLength / len);
    return position;
}

float light_balance(vec3 worldPos)
{
    vec4 lightClipPos = pc.lightProjView * vec4(worldPos, 1.0);
    vec3 lightNDC = lightClipPos.xyz / lightClipPos.w;
    float requiredDepth = lightNDC.z;
    vec2 lightPos2d = spread_clip_pos(lightNDC.xy);
    vec2 lightUV = lightPos2d * 0.5 + 0.5;
    float realDepth = 1;
    if (lightUV.x > 0 && lightUV.y > 0 && lightUV.x < 1 && lightUV.y < 1)
        realDepth = texture(iShadowmap, lightUV).r;
    return realDepth - requiredDepth;
}

vec3 world_space_normal()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;   // texture coordinates

    vec2 ndc = surf.wPos;
    vec3 normalVal = texture(iNormal, uv).xyz * 2 - 1;
    vec4 worldNormal = pc.invView * vec4(normalVal, 0);
    return worldNormal.xyz;
}

vec3 light_dir_ws()
{
    vec4 forward4 = pc.lightInvView * vec4(0, 0, 1, 0);
    return forward4.xyz;
}

void main()
{
    vec3 worldPos = world_position();
    float inLight = light_balance(worldPos);
    float lightAmount = max(0, dot(world_space_normal(), -light_dir_ws()));
    if (inLight > -0.0001)
        out_fragColor = vec4(vec3(lightAmount), 1.0);
    else
        out_fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}