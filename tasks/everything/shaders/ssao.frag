#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0) uniform sampler2D iDepth;
layout(binding = 1) uniform sampler2D iNormalEmissive;

layout(binding = 10) readonly buffer Kernel
{
    vec3 iKernelVectors[];
};

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;  // NDC coordinates in [-1,1] (Y up)
} surf;

layout(push_constant) uniform PushConstants
{
    mat4 proj;
    mat4 invProj;
} pc;

const float RADIUS = 0.03;
const uint SAMPLE_COUNT = 64;
const float BIAS = 0.01;

float random(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 uv = surf.wPos * 0.5 + 0.5;   // texture coordinates

    // 1. Reconstruct view-space position of current fragment
    vec2 ndc = surf.wPos;                         // NDC [-1,1]
    float depthVal = texture(iDepth, uv).r;       // NDC depth [0,1]
    vec4 clipPos = vec4(ndc, depthVal * 2.0 - 1.0, 1.0);
    vec4 viewPos4 = pc.invProj * clipPos;
    vec3 viewPos = viewPos4.xyz / viewPos4.w;     // view-space position

    // 2. View-space normal (already in [-1,1] range)
    vec3 N = normalize(texture(iNormalEmissive, uv).rgb * 2.0 - 1.0);

    // 3. Random rotation angle for the kernel
    float angle = random(uv) * 6.28318530718;  // 2*pi
    float ca = cos(angle);
    float sa = sin(angle);

    // Build orthonormal basis (tangent, bitangent) from normal
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 T = normalize(cross(up, N));
    vec3 B = cross(N, T);

    // Rotate T and B around N by the random angle
    vec3 T_rot = T * ca + B * sa;
    vec3 B_rot = cross(N, T_rot);   // ensures orthogonality

    float ao = 0.0;

    // 4. Sample the kernel
    for (uint i = 0; i < SAMPLE_COUNT; ++i) {
        // Transform kernel direction from tangent space to view space
        vec3 kernelVector = iKernelVectors[i];
        vec3 sampleDir = T_rot * kernelVector.x + B_rot * kernelVector.y + N * kernelVector.z;
        vec3 samplePos = viewPos + sampleDir * RADIUS;

        // Project sample position to NDC and then to texture coordinates
        vec4 clipSample = pc.proj * vec4(samplePos, 1.0);
        vec3 ndcSample = clipSample.xyz / clipSample.w;
        vec2 uvSample = ndcSample.xy * 0.5 + 0.5;

        // Discard samples outside the screen
        if (uvSample.x < 0.0 || uvSample.x > 1.0 ||
        uvSample.y < 0.0 || uvSample.y > 1.0) continue;

        // Sample depth at the projected location
        float depthSample = texture(iDepth, uvSample).r;

        // Reconstruct view-space Z of the occluder
        vec4 clipOccluder = vec4(ndcSample.xy, depthSample * 2.0 - 1.0, 1.0);
        vec4 viewOccluder = pc.invProj * clipOccluder;
        float occluderZ = viewOccluder.z / viewOccluder.w;

        // Compare depths (occluder closer than sample + bias)
        if (occluderZ > samplePos.z + BIAS || occluderZ < samplePos.z - RADIUS) {
            ao += 1.0;
        }
    }

    // Average and clamp
    ao /= float(SAMPLE_COUNT);
    ao = clamp(ao, 0.0, 1.0);

    out_fragColor.r = ao;
}