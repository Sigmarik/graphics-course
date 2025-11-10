#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.glsl"


layout(location = 0) in ivec2 vPos;

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  mat4 mModel;
} params;

struct TerrainDot
{
  float elevation;
  vec2 normal;
};

layout(std140, set = 0, binding = 0) readonly buffer Elevation
{
  TerrainDot terrainDots[];
};

struct ChunkBinding
{
  vec2 position;
  float size;
  uint offset;
};

layout(std140, set = 0, binding = 1) readonly buffer Bindings
{
  ChunkBinding chunkBindings[];
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
  ChunkBinding binding = chunkBindings[gl_InstanceIndex];
  vec2 lateralPos = vec2(vPos) / 16.0 * binding.size + binding.position;

  uint index = binding.offset + vPos.x * 17 + vPos.y;
  TerrainDot terrainPoint = terrainDots[index];

  const vec3 wPos = vec3(lateralPos.y, terrainPoint.elevation, lateralPos.x);
//  const vec3 wPos = vec3(lateralPos.y, float(index) / 16, lateralPos.x);
//  const vec3 wNorm = normalize(vec3(terrainPoint.normal.y, 1.0, terrainPoint.normal.x));
  const vec3 wNorm = normalize(vec3(0.0, 1.0, 0.0));
  const vec3 wTang = vec3(1.0, 0.0, 0.0);

  mat4 model = mat4(
  1, 0, 0, 0,
  0, 1, 0, 0,
  0, 0, 1, 0,
  0, 0, 0, 1);
  vOut.wPos   = (model * vec4(wPos, 1.0f)).xyz;
  vOut.wNorm  = normalize(mat3(transpose(inverse(model))) * wNorm);
  vOut.wTangent = normalize(mat3(transpose(inverse(model))) * wTang);
  vOut.texCoord = wPos.xz;

  gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
}
