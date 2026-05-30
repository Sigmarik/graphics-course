#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

const float CONTRAST_THRESHOLD = 0.1;
const float RANGE = 16;

layout(binding = 0) uniform sampler2D iSource;

layout(push_constant) uniform params_t
{
    uint enableFXAA;
} params;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

layout(location = 0) out vec4 out_fragColor;

float brightness(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float brightnessAt(vec2 uv)
{
    vec3 color = texture(iSource, uv).rgb;
    return brightness(color);
}

float difference(float b1, float b2)
{
    return (b1 - b2) / max(max(b1, b2), 0.01);
}

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;
    if (params.enableFXAA == 0)
    {
        out_fragColor = texture(iSource, uv);
        return;
    }

    vec2 texelSize = 1.0 / vec2(textureSize(iSource, 0));

    float coreBrightness = brightnessAt(uv);

    float contrastPX = difference(brightnessAt(uv + vec2(texelSize.x, 0.0)), coreBrightness);
    float contrastPY = difference(brightnessAt(uv + vec2(0.0, texelSize.y)), coreBrightness);
    float contrastNX = difference(brightnessAt(uv - vec2(texelSize.x, 0.0)), coreBrightness);
    float contrastNY = difference(brightnessAt(uv - vec2(0.0, texelSize.y)), coreBrightness);

    float maxContrastX = max(contrastPX, contrastNX);
    float maxContrastY = max(contrastPY, contrastNY);

    float maxContrast = max(maxContrastX, maxContrastY);

    if (maxContrast < CONTRAST_THRESHOLD)
    {
        out_fragColor = texture(iSource, uv);
        return;
    }

//    if (maxContrastX > maxContrastY)
//    {
//        out_fragColor = vec4(1.0, 0.1, 0.1, 1.0);
//    }
//    else
//    {
//        out_fragColor = vec4(0.1, 1.1, 1.0, 1.0);
//    }
//    return;

    ivec2 toBrightest = ivec2(0);
    if (contrastNX == maxContrast) toBrightest = ivec2(-1, 0);
    else if (contrastPX == maxContrast) toBrightest = ivec2(1, 0);
    else if (contrastNY == maxContrast) toBrightest = ivec2(0, -1);
    else toBrightest = ivec2(0, 1);

    ivec2 right = ivec2(-toBrightest.y, toBrightest.x);
    ivec2 left = -right;

    uint shiftL = 1;
    uint shiftR = 1;

    bool leftSign = false;
    bool rightSign = false;

    for (; shiftL < RANGE; ++shiftL)
    {
        float thisBrightness = brightnessAt(uv + left * texelSize * shiftL);
        float thatBrightness = brightnessAt(uv + left * texelSize * shiftL + toBrightest * texelSize);
        if (abs(difference(thisBrightness, thatBrightness)) < CONTRAST_THRESHOLD)
        {
            leftSign = difference(thisBrightness, coreBrightness) >= CONTRAST_THRESHOLD;
            break;
        }
    }

    for (; shiftR < RANGE; ++shiftR)
    {
        float thisBrightness = brightnessAt(uv + right * texelSize * shiftR);
        float thatBrightness = brightnessAt(uv + right * texelSize * shiftR + toBrightest * texelSize);
        if (abs(difference(thisBrightness, thatBrightness)) < CONTRAST_THRESHOLD)
        {
            rightSign = difference(thisBrightness, coreBrightness) >= CONTRAST_THRESHOLD;
            break;
        }
    }


    out_fragColor = vec4(0.0, float(shiftL + shiftR) / RANGE * 0.5, 0.0, 1.0);
//    return;

//    float thisBrightness = brightnessAt(uv);
//    float thatBrightness = brightnessAt(uv + vec2(toBrightest) * texelSize);
//    float diff = difference(thatBrightness, thisBrightness);
//    out_fragColor = vec4(diff);

    vec3 originalColor = texture(iSource, uv).rgb;
    vec3 offendingColor = texture(iSource, uv + toBrightest * texelSize).rgb;

    if (leftSign && !rightSign)
    {
        out_fragColor = vec4(mix(offendingColor, originalColor, float(shiftL) / (shiftL + shiftR)), 1.0);
    }
    else if (!leftSign && rightSign)
    {
        out_fragColor = vec4(mix(offendingColor, originalColor, float(shiftR) / (shiftL + shiftR)), 1.0);
    }
    else if (leftSign && rightSign)
    {
        float param = abs(float(shiftL) - float(shiftR)) / RANGE * 0.5;
        out_fragColor = vec4(mix(offendingColor, originalColor, param), 1.0);
    }
    else
    {
        out_fragColor = vec4(originalColor, 1.0);
    }

//    out_fragColor = vec4(vec3(float(shiftR) / (shiftL + shiftR)), 1.0);
}