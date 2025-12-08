#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(push_constant) uniform params_t
{
  mat4 mInverseProj;
  mat4 mView;
  uint mNumberOfLights;
} params;

struct PointLight
{
  vec3 position;
  vec3 color;
  float radius;
};

layout(std430, binding = 0) readonly buffer pointLights
{
  PointLight lights[];
};

layout(binding = 1) uniform sampler2D iAlbedo;
layout(binding = 2) uniform sampler2D iNormal;
layout(binding = 3) uniform sampler2D iDepth;

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
  vec2 wPos;
} surf;

void main()
{
  vec2 uv = surf.wPos / 2.0 + vec2(0.5);

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

  vec3 totalLight = vec3(0.0);

  for (int i = 0; i < params.mNumberOfLights; i++)
  {
    PointLight light = lights[i];
    vec4 viewSpaceLight = params.mView * vec4(light.position, 1.0);

    vec3 lightVector = viewPos.xyz - viewSpaceLight.xyz / viewSpaceLight.w;
    float relativeRadius = length(lightVector) / light.radius;
    float strength = max(0.0, 2.0 / (1.0 + relativeRadius) - 1.0);
    float diffuse = max(0.0, dot(-normalize(lightVector), viewSpaceNormal));

    totalLight += light.color * strength * diffuse;
  }

  out_fragColor.rgb = albedo * totalLight + albedo * 0.1;
  out_fragColor.a = 1.0f;
}
