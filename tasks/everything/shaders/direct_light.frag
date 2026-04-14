#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 0) uniform sampler2D iDepth;
layout(binding = 1) uniform sampler2D iNormal;
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

vec3 world_space_normal()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;
    vec3 normalVal = texture(iNormal, uv).xyz * 2.0 - 1.0;
    vec4 worldNormal = pc.invView * vec4(normalVal, 0.0);
    return worldNormal.xyz;
}

vec3 light_dir_ws()
{
    vec4 forward4 = pc.lightInvView * vec4(0.0, 0.0, 1.0, 0.0);
    return forward4.xyz;
}

// Screen‑space ray march for contact shadows.
// Returns 0.0 if occluded, 1.0 otherwise.
float contact_shadow(vec3 worldPos, vec3 lightDir, float maxDist, int steps)
{
    // Compute forward matrices (camera projection * view and view only)
    mat4 projView = inverse(pc.invProjView);
    mat4 view     = inverse(pc.invView);

    float stepSize = maxDist / float(steps);
    float occlusion = 1.0;

    for (int i = 1; i <= steps; ++i)
    {
        vec3 samplePos = worldPos + lightDir * (stepSize * float(i));

        // Project sample point to clip space, then NDC, then UV
        vec4 clipPos = projView * vec4(samplePos, 1.0);
        vec3 ndc = clipPos.xyz / clipPos.w;
        vec2 uv  = ndc.xy * 0.5 + 0.5;

        // Skip if outside screen
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        continue;

        float sampledDepth = texture(iDepth, uv).r;

        // Reconstruct world position of the sampled depth
        vec4 sampledWorldPos4 = pc.invProjView * vec4(ndc.xy, sampledDepth, 1.0);
        vec3 sampledWorld = sampledWorldPos4.xyz / sampledWorldPos4.w;

        // Compute actual world‑space distance from original point to occluder
        float distToOccluder = length(sampledWorld - worldPos);

        // If the occluder is farther than our contact threshold, ignore it
        if (distToOccluder > maxDist)
        continue;

        // Depth comparison (bias to avoid self‑occlusion)
        float bias = 0.0001;
        if (sampledDepth < ndc.z - bias)
        {
            occlusion = 0.0;
            break;
        }
    }
    return occlusion;
}

void main()
{
    vec3 worldPos = world_position();

    // Light direction towards the light source
    vec3 L = normalize(-light_dir_ws());

    float inLight = light_balance(worldPos);

    // Base directional lighting
    float lightAmount = max(0.0, dot(world_space_normal(), L));

    // Apply contact shadows only where the point is lit by the directional light
    if (inLight > -0.0001)
    {
        // Parameters: max distance (world units), number of steps
        float contact = contact_shadow(worldPos, L, 0.1, 8);
        lightAmount *= contact;
        out_fragColor = vec4(vec3(lightAmount), 1.0);
    }
    else
    {
        out_fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
