#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vPos;
layout(location = 1) in ivec3 vNorm;
layout(location = 2) in vec2 vUv;
layout(location = 3) in ivec3 vTangent;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mView;
} params;

struct InstanceInfo
{
    mat4 model;
    uint texIdx;
    vec3 fallbackDiffuse;
};

layout(std140, set = 0, binding = 0) readonly buffer InstanceInfoBuffer
{
    InstanceInfo instances[];
};


layout (location = 0 ) out VS_OUT
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
    vec4 wClipPos;
    vec3 fallbackDiffuse;
    flat uint texId;
} vOut;

out gl_PerVertex { vec4 gl_Position; };

void main(void)
{
    // We don't even need to divide vectors by 127 as they get normalized anyways.
    const vec4 wNorm = vec4(vNorm, 0.0f);
    const vec4 wTang = vec4(vTangent, 0.0f);

    InstanceInfo instance = instances[gl_InstanceIndex];

    mat4 model = instance.model;

    vOut.wPos   = (model * vec4(vPos, 1.0f)).xyz;
    vOut.wNorm  = normalize(mat3(transpose(inverse(model))) * wNorm.xyz);
    vOut.wNorm = normalize((params.mView * vec4(vOut.wNorm, 0.0f)).xyz);
    vOut.wTangent = normalize(mat3(transpose(inverse(model))) * wTang.xyz);
    vOut.texCoord = vUv;
    vOut.fallbackDiffuse = instance.fallbackDiffuse;
    vOut.texId = instance.texIdx;

    vOut.wClipPos = params.mProjView * vec4(vOut.wPos, 1.0);
    gl_Position = vOut.wClipPos;
}
