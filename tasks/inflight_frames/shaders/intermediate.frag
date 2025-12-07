#version 450

layout(location = 0) out vec4 out_fragColor;

layout(location = 0) in VS_OUT
{
    vec2 wPos;
} surf;

layout(binding = 0, set = 0) uniform AppData
{
    ivec2 iResolution;
    vec2 iMouse;
    float iTime;
};

vec3 rgba(int red, int green, int blue, int alpha)
{
    return vec3(float(red) / 256.0, float(green) / 256.0, float(blue) / 256.0);
}

void main() {
    vec3 kYellow = rgba(255, 210, 47, 1);
    vec3 kRed = rgba(255, 92, 92, 1);
    vec3 kBlue = rgba(77, 213, 231, 1);
    vec3 kWhite = rgba(235, 235, 235, 1);


    float yaw = (-surf.wPos.x + 1.0) * 180.0;
    float pitch = surf.wPos.y * 180.0;

    float distanceFromPole = 180.0 - abs(pitch);

    const float kPimpSize = 40.0 + sin(iTime * 5.0) * 20.0;

    if (distanceFromPole < kPimpSize)
    {
        out_fragColor = vec4(kWhite, 1.0);
        return;
    }

    vec3 color = kWhite;

    float section = yaw / 360.0;
    int segment = int(section * 6.0);

    if (segment % 2 == 0)
    {
        color = kWhite;
    }
    else if (segment == 1)
    {
        color = kRed;
    }
    else if (segment == 3)
    {
        color = kYellow;
    }
    else if (segment == 5)
    {
        color = kBlue;
    }

    color -= vec3(1.0 / ((distanceFromPole - kPimpSize) * 10.0 + 9.0));

    out_fragColor = vec4(color, 1.0);
}