#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0) uniform sampler2D iDepth;
layout(set = 1, binding = 0) uniform sampler2D iShadowmaps[];

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

layout(location = 0) out vec4 out_fragColor;

layout(push_constant) uniform PushConstants
{
    mat4 invProjView;
    mat4 lightProjView;
    mat4 invView;
    mat4 lightInvView;
} pc;

// Reconstruct world position from depth buffer
vec3 world_position()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;
    vec2 ndc = surf.wPos;
    float depthVal = texture(iDepth, uv).r;
    vec4 clipPos = vec4(ndc, depthVal, 1.0);
    vec4 worldPos4 = pc.invProjView * clipPos;
    return worldPos4.xyz / worldPos4.w;
}

// Cascade selection for shadow map lookup
vec2 spread_clip_pos(vec2 position)
{
    return position;
}

float light_balance(vec3 worldPos)
{
    vec4 lightClipPos = pc.lightProjView * vec4(worldPos, 1.0);
    vec3 lightNDC = lightClipPos.xyz / lightClipPos.w;
    float requiredDepth = lightNDC.z;
    vec2 lightPos2d = spread_clip_pos(lightNDC.xy);
    float realDepth = 1.0;

    uint smapIndex = 0;
    for (; smapIndex < 4; ++smapIndex)
    {
        if (abs(lightPos2d.x) < 1.0 && abs(lightPos2d.y) < 1.0) break;
        lightPos2d /= 3.0;
    }
    vec2 lightUV = lightPos2d * 0.5 + 0.5;
    if (lightUV.x > 0.0 && lightUV.y > 0.0 && lightUV.x < 1.0 && lightUV.y < 1.0)
    realDepth = texture(iShadowmaps[smapIndex], lightUV).r;

    return realDepth - requiredDepth;
}

vec3 light_dir_ws()
{
    vec4 forward4 = pc.lightInvView * vec4(0.0, 0.0, 1.0, 0.0);
    return forward4.xyz;
}

vec3 camera_pos_ws()
{
    vec4 clipPos = vec4(0, 0, 0, 1);
    vec4 worldPos4 = pc.invView * clipPos;
    return worldPos4.xyz / worldPos4.w;
}

float henyey_greenstain_phase(float cosPhi, float anisotropy)
{
    // Clamp anisotropy to valid range [-1, 1] to avoid numerical issues
    float g = clamp(anisotropy, -1.0, 1.0);
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosPhi;

    // Equivalent to pow(denom, 1.5) but more efficient
    float denom_sqrt = sqrt(denom);
    float denom_3_2 = denom_sqrt * denom_sqrt * denom_sqrt;  // denom^(3/2)

    return (1.0 - g2) / (4.0 * 3.141592653589793 * denom_3_2);
}

float exponential_fog(float densityIntegral)
{
    return exp(-densityIntegral);
}

float sample_curve(float percentage)
{
    return percentage * percentage;
}

void main()
{
    vec3 worldPos = world_position();
    vec3 camPos = camera_pos_ws();
    vec3 camVector = normalize(worldPos - camPos);

    const uint STEP_COUNT = 100;
    const vec3 FOG_COLOR = vec3(0.1);
    const vec3 WORLD_AMBIENT = vec3(0.3);

    float rayLength = distance(worldPos, camPos);

    // Light direction towards the light source
    vec3 L = normalize(-light_dir_ws());

    vec3 totalLight = vec3(0);
    float totalDensity = 0;

    for (uint stepIdx = 1; stepIdx <= STEP_COUNT; ++stepIdx)
    {
        float thisInterpParam = sample_curve(float(stepIdx) / STEP_COUNT);
        float prevInterpParam = sample_curve(float(stepIdx - 1) / STEP_COUNT);
        float stepDistance = (thisInterpParam - prevInterpParam) * rayLength;
        vec3 currentWorldPos = camPos + camVector * thisInterpParam * rayLength;

        float thisPointDensity = 0.5;

        bool inLight = bool(light_balance(currentWorldPos) > 0.0);
        vec3 light = FOG_COLOR * WORLD_AMBIENT;
        if (inLight)
        {
            light += FOG_COLOR * henyey_greenstain_phase(dot(L, camVector), 0.5) * 10;
        }
        totalDensity += thisPointDensity * stepDistance;
        totalLight += light * thisPointDensity * stepDistance * exponential_fog(totalDensity);
    }

    out_fragColor = vec4(totalLight, exponential_fog(totalDensity));
}
