#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require


layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_depth;

layout(location = 0) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
  vec4 wClipPos;
} surf;

void main()
{
  const vec3 surfaceColor = vec3(1.0f, 1.0f, 1.0f);

  out_albedo = vec4(surfaceColor, 1.0);
  out_normal.xyz = normalize(surf.wNorm) / 2.0 + vec3(0.5);
  out_depth.r = surf.wClipPos.z / surf.wClipPos.w / 2.0 + 0.5;
}
