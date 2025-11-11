#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require


layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
} surf;

const vec3 kSunVector = vec3(5.0, 10.0, 2.0);

vec3 colorBySlope(float slope)
{
  return slope > 0.7 ? vec3(0.3, 0.3, 0.35) : vec3(0.8, 0.8, 0.85);
}

void main()
{
  vec3 normal = surf.wNorm;
  const vec3 surfaceColor = colorBySlope(length(normal.xz) / normal.y);

  const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

  const vec3 lightDir   = normalize(kSunVector);
  const vec3 diffuse = max(dot(normal, lightDir), 0.0f) * lightColor;
  const float ambient = 0.4;
  out_fragColor.rgb = (diffuse * (1.0 - ambient) + ambient) * surfaceColor;
  out_fragColor.a = 1.0f;
}
