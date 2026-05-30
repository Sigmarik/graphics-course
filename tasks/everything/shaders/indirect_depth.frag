#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

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

void main()
{
    // Do nothing?
}
