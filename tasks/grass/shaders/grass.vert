#version 450
#extension GL_ARB_separate_shader_objects : enable

const float CELL_SIZE = 0.1;
const float SCALE_MIN = 0.9;
const float SCALE_MAX = 1.1;

layout(location = 0) in vec3 vPos;
layout(location = 1) in float vHeight;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
} params;

layout (location = 0 ) out VS_OUT
{
    float growth;
} vOut;

out gl_PerVertex { vec4 gl_Position; };

vec2 intersectPlane(vec3 start, vec3 end)
{
    float t = -start.y / (end.y - start.y);
    return start.xz + t * (end.xz - start.xz);
}

vec2 intersectPlane(vec3 start, vec3 end, vec3 shift)
{
    return intersectPlane(start + shift, end + shift);
}

void viewRectangle(out vec2 bottomLeft, out vec2 bottomRight, out vec2 topLeft, out vec2 topRight)
{
    mat4 invProjView = inverse(params.mProjView);

    vec4 bl = invProjView * vec4(-1.0, -1.0, -1.0, 1.0);
    vec4 br = invProjView * vec4(1.0, -1.0, -1.0, 1.0);
    vec4 tl = invProjView * vec4(-1.0, -1.0 + 0.1, -1.0, 1.0);
    vec4 tr = invProjView * vec4(1.0, -1.0 + 0.1, -1.0, 1.0);

    vec4 origin = invProjView * vec4(0.0, 0.0, 0.0, 1.0);

    bl /= bl.w;
    br /= br.w;
    tl /= tl.w;
    tr /= tr.w;

    origin /= origin.w;

    float padding = 0.3;
    vec3 paddingX = -normalize(tr.xyz - tl.xyz) * padding;
    vec3 paddingY = -normalize(tr.xyz - br.xyz) * padding;

    bottomLeft = intersectPlane(origin.xyz, bl.xyz, -paddingX - paddingY);
    bottomRight = intersectPlane(origin.xyz, br.xyz, paddingX - paddingY);
    topLeft = intersectPlane(origin.xyz, tl.xyz, -paddingX);
    topRight = intersectPlane(origin.xyz, tr.xyz, paddingX);
}

uint cellIntegral(float bottomSize, float topSize, float bottomToTopDistance, float roughDistance)
{
    float currentSize = bottomSize + (topSize - bottomSize) * roughDistance / bottomToTopDistance;
    float exactIntegral = (bottomSize + currentSize) / 2 * roughDistance;
    return uint(exactIntegral);
}

// Frustum-based instance generation, works better than every other technique
// combined in my oppinion, and is very simple to implement.
// Sorry to disappoint if you wanted to see hot byte-on-byte buffer fapping or
// something, I wold much rather practice methods that actually work.
ivec2 instanceCell()
{
    vec2 bottomLeft = vec2(-20, -20);
    vec2 bottomRight = vec2(20, -20);
    vec2 topLeft = vec2(-30, 20);
    vec2 topRight = vec2(30, 20);
    viewRectangle(bottomLeft, bottomRight, topLeft, topRight);

    bottomLeft /= CELL_SIZE;
    bottomRight /= CELL_SIZE;
    topLeft /= CELL_SIZE;
    topRight /= CELL_SIZE;

    float bottomSize = distance(bottomLeft, bottomRight);
    float topSize = distance(topLeft, topRight);
    vec2 frustumAxis = (topLeft + topRight) * 0.5 - (bottomLeft + bottomRight) * 0.5;
    float bottomToTopDistance = length(frustumAxis);

    // Because I rely on integrals and linear algebra and not exact cell structure,
    // I need to "scew" indices a little to avoid grass flickering.
    float perceivedIndex = gl_InstanceIndex * 0.4;

    // Boonga need roughDistance. Boonga derive roughDistance from equation:

    // (bottomSize * 2 + (topSize - bottomSize) * roughDistance / bottomToTopDistance) / 2 * roughDistance ~= gl_InstanceID
    // roughDistance > 0

    float roughDistance = perceivedIndex / bottomSize;
    if (bottomSize != topSize)
    {
        roughDistance = (
                bottomSize * bottomToTopDistance -
                sqrt(bottomToTopDistance * (
                    bottomSize * bottomSize * bottomToTopDistance +
                    2 * perceivedIndex * (topSize - bottomSize)
                ))
            ) / (bottomSize - topSize);
    }

    uint cellCountMin = cellIntegral(bottomSize, topSize, bottomToTopDistance, floor(roughDistance));
    uint cellCountMax = cellIntegral(bottomSize, topSize, bottomToTopDistance, ceil(roughDistance));

    float horizontalCoefficient = float(perceivedIndex - cellCountMin) / (cellCountMax - cellCountMin) - 0.5;

//    horizontalCoefficient = 0.5;
//    roughDistance = gl_InstanceIndex;

    vec2 roughPosition = (bottomLeft + bottomRight) * 0.5 + normalize(frustumAxis) * floor(roughDistance) +
        (bottomSize + (topSize - bottomSize) * floor(roughDistance) / bottomToTopDistance) * horizontalCoefficient *
        normalize(bottomRight - bottomLeft);

    return ivec2(roughPosition);
}

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

mat4 instanceTransform()
{
    ivec2 cell = instanceCell();

    vec2 cellF = vec2(cell);

    float rX = random(cellF);
    float rY = random(cellF + vec2(1.0, 0.0));
    float rA = random(cellF + vec2(0.0, 1.0));
    float rS = random(cellF + vec2(1.0, 1.0));

    vec2 base = vec2(cell) * CELL_SIZE;
    vec2 offset = vec2(rX, rY) * CELL_SIZE;
    vec3 translation = vec3(base + offset, 0.0);
    translation = translation.xzy;

    float angle = rA * 2.0 * 3.14159265359;

    float scale = SCALE_MIN + rS * (SCALE_MAX - SCALE_MIN);

    float rotCos = cos(angle);
    float rotSin = sin(angle);

    return mat4(
        vec4(scale * rotCos, 0.0, scale * rotSin, 0.0),
        vec4(0.0, scale, 0.0, 0.0),
        vec4(-scale * rotSin, 0.0, scale * rotCos, 0.0),
        vec4(translation, 1.0)
    );
}

void main(void)
{
//    InstanceInfo instance = instances[gl_InstanceIndex];

    mat4 model = instanceTransform();

    vec3 pos = (model * vec4(vPos, 1.0f)).xyz;
    vOut.growth = vHeight;
    gl_Position = params.mProjView * vec4(pos, 1.0);
}
