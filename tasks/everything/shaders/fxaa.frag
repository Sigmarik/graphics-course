#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

const float CONTRAST_THRESHOLD = 0.1;

layout(binding = 0) uniform sampler2D iSource;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

layout(location = 0) out vec4 out_fragColor;

float brightnessAt(vec2 uv)
{
    vec3 color = texture(iSource, uv).rgb;
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;
    vec2 texelSize = 1.0 / vec2(textureSize(iSource, 0));

    float contrastPX = brightnessAt(uv + vec2(texelSize.x, 0.0)) - brightnessAt(uv);
    float contrastPY = brightnessAt(uv + vec2(0.0, texelSize.y)) - brightnessAt(uv);
    float contrastNX = brightnessAt(uv - vec2(texelSize.x, 0.0)) - brightnessAt(uv);
    float contrastNY = brightnessAt(uv - vec2(0.0, texelSize.y)) - brightnessAt(uv);

    float maxContrastX = max(abs(contrastPX), abs(contrastPY));
    float maxContrastY = max(abs(contrastNX), abs(contrastNY));

    if (true || maxContrastX < CONTRAST_THRESHOLD && maxContrastY < CONTRAST_THRESHOLD)
    {
        out_fragColor = texture(iSource, uv);
        return;
    }

    if (maxContrastX > maxContrastY)
    {
        out_fragColor = vec4(1.0, 1.0, 0.1, 1.0);
    }
    else
    {
        out_fragColor = vec4(1.0, 0.1, 1.0, 1.0);
    }
}