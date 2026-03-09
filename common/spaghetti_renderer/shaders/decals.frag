#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform params_t
{
  mat4 mInverseProj;
  mat4 mView;
  uint mNumberOfDecals;
} params;

struct Decal
{
  vec3 position;
  vec3 direction;
  float size;
  float orientation;
  float depth;
};

layout(std430, binding = 0) readonly buffer decalBuffer
{
  Decal decals[];
};

layout(binding = 1) uniform sampler2D iAlbedo;
layout(binding = 2) uniform sampler2D iNormal;
layout(binding = 3) uniform sampler2D iDepth;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_depth;

layout(location = 0) in VS_OUT
{
  vec2 wPos;
} surf;

void decalTexture(vec2 pos, in out vec3 albedo, in out vec3 normal)
{
  int cells = 8;
  ivec2 position = ivec2(floor(pos.x * cells), floor(pos.y * cells));

  if ((position.x + position.y) % 2 == 0) albedo = albedo * 0.5;
}

void main()
{
  vec2 uv = surf.wPos / 2.0 + vec2(0.5);

  out_albedo = texture(iAlbedo, uv);
  out_normal = texture(iNormal, uv);
  out_depth = texture(iDepth, uv);

  vec3 albedo = texture(iAlbedo, uv).rgb;
  float depth = texture(iDepth, uv).r;
  if (depth == 0.0) discard;
  vec3 normal = texture(iNormal, uv).rgb * 2.0 - vec3(1.0);

  vec2 ndcXY = surf.wPos;
  float ndcZ = depth * 2.0 - 1.0;

  vec4 clipPos = vec4(ndcXY, ndcZ, 1.0);
  vec4 viewPosH = params.mInverseProj * clipPos;
  vec3 viewPos = viewPosH.xyz / viewPosH.w;
  vec3 viewSpaceNormal = normalize((params.mView * vec4(normal, 0.0)).xyz);

  vec3 viewUp = normalize((params.mView * vec4(0.001234567, 1.213141, 0.0, 0.0)).xyz);

  for (uint decalIdx = 0; decalIdx < params.mNumberOfDecals; ++decalIdx)
  {
    Decal decal = decals[decalIdx];

    vec4 viewSpaceDecalPos = params.mView * vec4(decal.position, 1.0);
    vec3 viewSpaceDirection = normalize((params.mView * vec4(decal.direction, 0.0)).xyz);

    vec3 decalRight = normalize(cross(viewSpaceDirection, viewUp));
    vec3 decalUp = normalize(cross(viewSpaceDirection, decalRight));

    vec3 orientedRight = decalRight * cos(decal.orientation) + decalUp * sin(decal.orientation);
    vec3 orientedUp = decalUp * cos(decal.orientation) - decalRight * sin(decal.orientation);

    vec3 relativeFragmentPos = viewPos - viewSpaceDecalPos.xyz;

    vec2 decalPosition = vec2(dot(relativeFragmentPos, orientedRight), dot(relativeFragmentPos, orientedUp));
    decalPosition /= decal.size;

    if (abs(dot(relativeFragmentPos, viewSpaceDirection)) * 2.0 > decal.depth) continue;

    if (decalPosition.x < -0.5 || decalPosition.x > 0.5 || decalPosition.y < -0.5 || decalPosition.y > 0.5) continue;

    decalTexture(decalPosition, albedo, normal);
  }

  out_albedo = vec4(albedo, 1.0);
  out_normal = vec4(normal / 2.0 + vec3(0.5), 1.0);
  out_depth = vec4(depth, 0.0, 0.0, 0.0);
}
