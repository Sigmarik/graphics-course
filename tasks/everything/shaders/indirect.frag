#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normalEmissive;

layout(location = 0) in VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
    vec4 wClipPos;
    vec3 fallbackDiffuse;
    flat uint texId;
} surf;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    const vec3 surfaceColor = surf.texId == 0 ?
        surf.fallbackDiffuse :
        texture(textures[nonuniformEXT(surf.texId - 1)], surf.texCoord).rgb;

    out_albedo = vec4(surfaceColor, 1.0f);
    out_normalEmissive = vec4(surf.wNorm * 0.5 + 0.5, 0.0f);
}
