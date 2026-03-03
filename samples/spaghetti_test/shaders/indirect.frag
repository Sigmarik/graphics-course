#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_fragColor;

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
    const vec3 wLightPos = vec3(10, 10, 10);
    const vec3 surfaceColor = surf.texId == 0 ?
        surf.fallbackDiffuse :
        texture(textures[nonuniformEXT(surf.texId - 1)], surf.texCoord).rgb;

    const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    const vec3 lightDir   = normalize(wLightPos - surf.wPos);
    const vec3 diffuse = max(dot(surf.wNorm, lightDir), 0.0f) * lightColor;
    const float ambient = 0.3;
    out_fragColor.rgb = (diffuse + ambient) * surfaceColor;
    out_fragColor.a = 1.0f;
//    out_fragColor = vec4(float(surf.texId) / 2.0, 0.0, 0.0, 1.0);
}
