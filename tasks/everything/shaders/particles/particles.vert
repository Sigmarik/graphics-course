#version 460
#extension GL_EXT_shader_8bit_storage : require
#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
struct Particle {
    float px, py, pz;
    float vx, vy, vz;
    float ttl;
    float size;
    uint8_t r, g, b, a;
};
layout(std430, binding = 0) buffer ParticlesBuffer { Particle particles[]; };
layout(std430, binding = 1) buffer SortedIndices { uint16_t indices[]; };
layout(push_constant) uniform PushConstants {
    mat4 globalToScreen;
} push;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
void main() {
    uint idx = indices[gl_InstanceIndex];
    Particle p = particles[idx];
    if (p.ttl <= 0.0) {
        gl_Position = vec4(10.0, 10.0, -10.0, 1.0); // Outside of clip space
        return;
    }
    vec2 uv = vec2(gl_VertexIndex & 1, (gl_VertexIndex >> 1) & 1);
    outUV = uv;
    outColor = vec4(float(p.r)/255.0, float(p.g)/255.0, float(p.b)/255.0, float(p.a)/255.0);
    vec4 pos = push.globalToScreen * vec4(p.px, p.py, p.pz, 1.0);
    vec2 offset = (uv - 0.5) * 2.0; 
    pos.xy += offset * p.size * pos.w;
    gl_Position = pos;
}
