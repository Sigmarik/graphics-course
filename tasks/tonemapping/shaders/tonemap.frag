#version 450

layout(binding = 0) uniform sampler2D iSource;

layout(location = 0) out vec4 out_fragColor;

layout(push_constant) uniform params
{
  uint resolutionX;
  uint resolutionY;
  float avgBrightness;
  float varBrightness;
} params_t;

layout(location = 0) in VS_OUT
{
  vec2 wPos;
} surf;

ivec3 iResolution()
{
  return ivec3(params_t.resolutionX, params_t.resolutionY, 0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = fragCoord / iResolution().xy;

  vec4 source = texture(iSource, uv);

  vec3 color = source.rgb;
  float value = length(source.rgb);
  float transformedBrightness = (value - params_t.avgBrightness) / params_t.varBrightness / 3.0 + 0.6;
  fragColor = vec4(color * transformedBrightness, 1.0);
}

void main()
{
  vec2 pos = surf.wPos / 2.0 + vec2(0.5);
//  pos.y = 1.0 - pos.y;
  pos = pos * vec2(iResolution());

  vec4 fragColor = vec4(0.0);
  mainImage(fragColor, pos);

  out_fragColor = fragColor;
}
