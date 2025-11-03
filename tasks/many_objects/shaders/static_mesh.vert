#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.glsl"


layout(location = 0) in vec3 vPos;
layout(location = 1) in ivec3 vNorm;
layout(location = 2) in vec2 vUv;
layout(location = 3) in ivec3 vTangent;

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  mat4 mModel;
} params;

layout(set = 0, binding = 0) uniform Matrices
{
  mat4 instanceMatrices[512];
};


layout (location = 0 ) out VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
} vOut;

out gl_PerVertex { vec4 gl_Position; };

void main(void)
{
  // We don't even need to divide vectors by 127 as they get normalized anyways.
  const vec4 wNorm = vec4(vNorm, 0.0f);
  const vec4 wTang = vec4(vTangent, 0.0f);

  mat4 model = instanceMatrices[gl_InstanceIndex];
  vOut.wPos   = (model * vec4(vPos, 1.0f)).xyz;
  vOut.wNorm  = normalize(mat3(transpose(inverse(model))) * wNorm.xyz);
  vOut.wTangent = normalize(mat3(transpose(inverse(model))) * wTang.xyz);
  vOut.texCoord = vUv;

  gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
}
