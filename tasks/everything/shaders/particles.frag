#version 460

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outFragColor;

void main() {
    float dist = length(inUV - 0.5) * 2.0;
    if (dist > 1.0) discard;

    float alpha = inColor.a * (1.0 - smoothstep(0.8, 1.0, dist));
    outFragColor = vec4(inColor.rgb, alpha);
}
