#version 460

struct Particle {
    float px, py, pz;
    float vx, vy, vz;
    float ttl;
    float size;
    uint colorRGBA;
};

layout(std430, binding = 0) readonly buffer ParticlesBuffer { Particle particles[]; };
layout(std430, binding = 1) readonly buffer SortedIndices { uint indices[]; };

layout(push_constant) uniform PushConstants {
    mat4 globalToScreen;
    float aspect;          // screen width / screen height
} push;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;

vec4 unpackColor(uint packed) {
    float r = float(packed & 0xFFu) / 255.0;
    float g = float((packed >> 8) & 0xFFu) / 255.0;
    float b = float((packed >> 16) & 0xFFu) / 255.0;
    float a = float((packed >> 24) & 0xFFu) / 255.0;
    return vec4(r, g, b, a);
}

void main() {
    uint idx = indices[gl_InstanceIndex];
    Particle p = particles[idx];

    if (p.ttl <= 0.0) {
        gl_Position = vec4(10.0, 10.0, -10.0, 1.0);
        return;
    }

    vec2 uv = vec2(gl_VertexIndex & 1, (gl_VertexIndex >> 1) & 1);
    outUV = uv;
    outColor = unpackColor(p.colorRGBA);

    vec4 pos = push.globalToScreen * vec4(p.px, p.py, p.pz, 1.0);
    vec2 offset = (uv - 0.5) * 2.0;
    offset.x /= push.aspect;          // ✅ fix aspect ratio distortion
    pos.xy += offset * p.size;        // constant world‑space size
    gl_Position = pos;
}