#version 450
#extension GL_ARB_separate_shader_objects : enable

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

ivec2 instanceCell() {
    int n = gl_InstanceIndex;
    if (n == 0) return ivec2(0);

    // Find ring k such that (2k-1)^2 <= n < (2k+1)^2
    int k = int(floor((sqrt(float(n)) + 1.0) / 2.0));
    int start = (2 * k - 1) * (2 * k - 1);
    int offset = n - start;
    int side = 2 * k; // number of cells on each side of the ring

    int x, y;
    if (offset < side) { // right side, moving up
                         x = k;
                         y = 1 - k + offset;
    } else if (offset < 2 * side) { // top side, moving left
                                    int t = offset - side;
                                    x = k - 1 - t;
                                    y = k;
    } else if (offset < 3 * side) { // left side, moving down
                                    int t = offset - 2 * side;
                                    x = -k;
                                    y = k - 1 - t;
    } else { // bottom side, moving right
             int t = offset - 3 * side;
             x = -k + 1 + t;
             y = -k;
    }
    return ivec2(x, y);
}

const float CELL_SIZE = 0.1;
const float SCALE_MIN = 0.9;
const float SCALE_MAX = 1.1;

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
