#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    float growth;
} surf;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main()
{
    vec3 surfaceColor = mix(vec3(0.0), vec3(1.0), surf.growth);
    float ambient = 1.0f;
    float diffuse = 0.0f;
    out_fragColor.rgb = (diffuse + ambient) * surfaceColor;
    out_fragColor.a = 1.0f;
}
