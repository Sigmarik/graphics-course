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
  ivec2 position;
  uint size;
  uint offset;
};

layout(std140, set = 0, binding = 1) readonly buffer Bindings
{
  ChunkBinding chunkBindings[];
};


layout (location = 0) out VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
  float dist;
} vOut;

out gl_PerVertex { vec4 gl_Position; };

const uint CHUNK_RESOLUTION = 5;

void main(void)
{
  // We don't even need to divide vectors by 127 as they get normalized anyways.
  ChunkBinding binding = chunkBindings[gl_InstanceIndex];
  vec2 lateralPos = vec2(vPos) / (CHUNK_RESOLUTION - 1) * binding.size + binding.position;

  uint index = binding.offset + vPos.x * CHUNK_RESOLUTION + vPos.y;
  TerrainDot terrainPoint = terrainDots[index];

  const vec3 wPos = vec3(lateralPos.y, terrainPoint.elevation, lateralPos.x);
  float normalLength = length(terrainPoint.normal);
  const vec3 wNorm = vec3(terrainPoint.normal.y, sqrt(1.0 - normalLength * normalLength), terrainPoint.normal.x);
  const vec3 wTang = vec3(1.0, 0.0, 0.0);

  vOut.wPos   = wPos.xyz;
  vOut.wNorm  = normalize(wNorm);
  vOut.wTangent = normalize(wTang);
  vOut.texCoord = wPos.xz;
  gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
  vOut.dist = gl_Position.z;
}
