#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D iColor;
layout(binding = 1) uniform sampler2D iDepth;
layout(binding = 2) uniform sampler2D iReflection;

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

layout(push_constant) uniform PushConstants
{
    mat4 invProjView;
    mat4 projView;
    float waterLevel;
    vec3 _pad0;
} pc;

vec3 getCameraPos()
{
    vec4 origin = pc.invProjView * vec4(0.0, 0.0, 0.0, 1.0);
    return origin.xyz / origin.w;
}

vec3 getWorldPositionHere()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;
    float depthVal = texture(iDepth, uv).r;
    vec4 clipPos = vec4(surf.wPos, depthVal, 1.0);
    vec4 worldPos4 = pc.invProjView * clipPos;
    return worldPos4.xyz / worldPos4.w;
}

vec3 getRayDirection()
{
    vec4 clipPos = vec4(surf.wPos, 1, 1.0);
    vec4 worldPos4 = pc.invProjView * clipPos;
    return normalize(worldPos4.xyz / worldPos4.w - getCameraPos());
}

vec3 worldPosToClip(vec3 worldPos)
{
    vec4 clipPos = pc.projView * vec4(worldPos, 1.0);
    return clipPos.xyz / clipPos.w;
}

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;

    float baseDepth = texture(iDepth, uv).r;
    vec3 baseColor = texture(iColor, uv).rgb;
    uv.x = 1.0 - uv.x;
    vec3 reflColor = texture(iReflection, uv).rgb;

    vec3 camPos = getCameraPos();
    vec3 rayDir = getRayDirection();
    vec3 worldPosHere = getWorldPositionHere();

    vec3 finalColor = baseColor;

    float t = (camPos.y - pc.waterLevel) / rayDir.y;
    vec3 hitPoint = camPos - t * rayDir;
    if (t > 0.0 || (baseDepth < 1.0 && distance(hitPoint, camPos) >= distance(worldPosHere, camPos)))
    {
        out_fragColor = vec4(baseColor, 1.0);
        gl_FragDepth = baseDepth;
        return;
    }
    gl_FragDepth = worldPosToClip(hitPoint).z * 0.5 + 0.5;

    finalColor = reflColor;

    out_fragColor = vec4(finalColor, 1.0);
}