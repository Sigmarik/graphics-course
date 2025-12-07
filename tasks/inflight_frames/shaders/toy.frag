#version 450

layout(binding = 0) uniform sampler2D iBallTexture;
layout(binding = 1) uniform sampler2D iSkyTexture;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 2, set = 0) uniform AppData
{
  ivec2 iResolution;
  vec2 iMouse;
  float iTime;
};

layout(location = 0) in VS_OUT
{
  vec2 wPos;
} surf;

const vec3 kUp = vec3(0.0, 1.0, 0.0);
const float kEps = 0.01;

const float kBallRadius = 0.2;

float smoothNoise(vec3 pos, float speed)
{
  float time = iTime * 3.0 * speed;
  return (
  sin(pos.x * 1.7 + pos.z * 0.3 + time) * 0.3 +
  cos(pos.x * 1.3 + pos.z * 0.5 + time * 0.7) * 0.2 +
  cos(pos.x * 0.4 + pos.z * 1.5 - time * 0.3) * 0.4 +
  cos(-pos.x * 0.4 + pos.z * 1.5 + time * 1.2) * 0.3
  ) / 2.0 + 0.5;
}

vec3 ballPos()
{
  float delta = smoothNoise(vec3(0.0), 1.0) - 0.5;
  delta *= 0.07;
  return vec3(0.0, delta + 0.07, 0.0);
}

float ball(vec3 pos)
{
  return distance(pos, ballPos()) - kBallRadius;
}

vec3 rgba(int red, int green, int blue, int alpha)
{
  return vec3(float(red) / 256.0, float(green) / 256.0, float(blue) / 256.0);
}

const float kPi = 3.1415926535;

vec2 getLookDirection(vec3 vector)
{
  float yaw = atan(vector.z, vector.x) / kPi / 2.0;
  float pitch = atan(vector.y, length(vector.xz)) / kPi + 0.5;
  return vec2(yaw, 1.0 - pitch);
}

vec3 ballColor(vec3 worldNormal)
{
  mat3 rotation = mat3(
  0.9659258, -0.2588190, -0.0000000,
  0.2241439,  0.8365163, -0.5000000,
  0.1294095,  0.4829629,  0.8660254);

  vec3 normal = rotation * worldNormal;
  vec2 direction = getLookDirection(normal);

  return texture(iBallTexture, direction).rgb;
}

float waterPlane(vec3 pos)
{
  float waves = smoothNoise(pos * 100.0, 1.0) * 0.004 +
  smoothNoise(pos * 40.0, 1.0) * 0.02 +
  smoothNoise(pos * 10.0, 1.0) * 0.1;
  return pos.y - waves / (1.0 + max(0.0, length(pos.xz) - 0.2) * 1.6);
}

float globalSdf(int object, vec3 pos)
{
  if (object == 1)
  {
    return waterPlane(pos);
  }
  else
  {
    return ball(pos);
  }
}

float trace(int object, vec3 start, vec3 ray, out vec3 position, out vec3 normal)
{
  ray = normalize(ray);

  int iterations = 850;

  vec3 pos = start;

  if (object == 0)
  {
    float projection = dot(ballPos() - start, ray);
    if (projection > 0.0)
    {
      pos += ray * projection;
      float delta = distance(pos, ballPos());
      if (delta < kBallRadius)
      {
        delta = sqrt(kBallRadius * kBallRadius - delta * delta);
        pos -= ray * delta;
      }
    }
  }

  if (object == 1 && ray.y < 0.0)
  {
    pos -= ray * (pos.y / ray.y);
  }

  float sdf = 0.0;
  for (int iter = 0; iter < iterations; ++iter)
  {
    sdf = globalSdf(object, pos);
    pos += normalize(ray) * sdf;

    if (object == 0 && abs(pos.z) > 3.0)
    break;

    if (object == 1 && ray.y >= 0.0)
    break;
  }

  position = pos;
  normal.x = globalSdf(object, pos + vec3(kEps, 0.0, 0.0)) - sdf;
  normal.y = globalSdf(object, pos + vec3(0.0, kEps, 0.0)) - sdf;
  normal.z = globalSdf(object, pos + vec3(0.0, 0.0, kEps)) - sdf;
  normal = normalize(normal);

  return sdf;
}

vec3 deepBlue(vec3 ray)
{
  vec3 zenith = rgba(113, 164, 219, 1);
  vec3 horizon = rgba(25, 51, 117, 1);
  float interpolation = pow(normalize(ray).y + 1.0, 3.0);

  vec3 color = zenith * interpolation + horizon * (1.0 - interpolation);

  return color;
}

vec3 smoothSky(vec3 ray)
{
  vec3 zenith = rgba(82, 140, 233, 1);
  vec3 horizon = rgba(185, 230, 244, 1);
  float interpolation = smoothstep(0.0, 1.0, normalize(ray).y / 2.0 + 0.5);

  vec3 color = zenith * interpolation + horizon * (1.0 - interpolation);

  return color;
}

vec3 sky(vec3 ray)
{
  vec2 dir = getLookDirection(normalize(ray));
  dir.x += 0.75;  // 1.0 = 360 degrees
  if (dir.x > 1.0) dir.x -= 1.0;
  vec3 color = texture(iSkyTexture, dir).rgb;

  float smoothing = 0.2;

  color.x = pow(color.x, smoothing);
  color.y = pow(color.y, smoothing);
  color.z = pow(color.z, smoothing);

  return color;
}

vec3 kSun = normalize(vec3(-0.2, -1.0, 0.2));

float specular(vec3 normal, vec3 view)
{
  // Blinn-Phong
  vec3 bisector = normalize(-view - kSun);
  return pow(max(0.0, dot(bisector, normal)), 170.0);
}

vec3 shade(vec3 normal, vec3 view)
{
  vec3 albedo = ballColor(normal);
  vec3 ao = albedo * (smoothSky(normal) + vec3(1.0)) / 2.0;
  vec3 direct = albedo * dot(normal, -kSun);
  vec3 spec = vec3(1.0) * specular(normal, view);
  return direct * 0.3 + ao * 0.9  + spec * 0.2;
}

vec3 colorCorrect(vec3 color)
{
  vec3 result = color;
  result = smoothstep(0.0, 1.0, result);
  return result;
}

vec3 shadeWater(vec3 pos, vec3 normal, vec3 view)
{
  vec3 refracted = refract(view, normal, 1.0 / 1.33);//normalize(mix(view, -normal, 0.05));
  vec3 reflected = reflect(view, normal);

  vec3 reflectedSky = sky(reflected);

  vec3 ballPosition;
  vec3 ballNormal;
  float dist = trace(0, pos, refracted, ballPosition, ballNormal);

  vec3 result = mix(deepBlue(refracted), rgba(115, 224, 238, 1), pos.y / 0.6);
  result = mix(result, reflectedSky, pow(1.0 - abs(view.y), 3.0) * 0.75);
  if (dist < kEps)
  {
    result = shade(ballNormal, refracted);
    vec3 mixedWaterColor = deepBlue(ballNormal);
    mixedWaterColor = mix(mixedWaterColor, rgba(55, 99, 180, 1), 0.4);
    // rgba(75, 137, 228, 1)
    result = mix(result, mixedWaterColor, 0.6);
  }

  if (ball(pos) < smoothNoise(pos * 100.0, 3.0) * 0.01 + 0.005)
  {
    result = rgba(216, 240, 243, 1);
  }

  result += specular(normal, view) * vec3(1.0) * 0.04;

  return result;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 mouse = iMouse.xy / iResolution.xy - vec2(0.5);

  vec3 cameraPos = normalize(vec3(mouse.x, mouse.y + 0.7, 1.0)) * 0.9;
  vec3 forward = normalize(vec3(0.0, 0.1, 0.0) - cameraPos);
  vec3 right = normalize(cross(forward, kUp));
  vec3 up = normalize(cross(right, forward));

  vec2 uv = fragCoord / iResolution;
  vec2 relativeScreen = (uv - 0.5) * iResolution / iResolution.x * 2.0;

  vec3 ray = forward + relativeScreen.x * right + relativeScreen.y * up;

  vec3 ballPosition;
  vec3 ballNormal;
  float ballDelta = trace(0, cameraPos, ray, ballPosition, ballNormal);

  vec3 waterPosition;
  vec3 waterNormal;
  float waterDelta = trace(1, cameraPos, ray, waterPosition, waterNormal);

  vec3 col = sky(ray);

  if ((distance(ballPosition, cameraPos) < distance(waterPosition, cameraPos) || waterDelta > kEps) &&
  ballDelta < kEps)
  {
    col = colorCorrect(shade(ballNormal, normalize(ballPosition - cameraPos)));
  }
  else if (waterDelta < kEps)
  {
    col = colorCorrect(shadeWater(waterPosition, waterNormal, normalize(waterPosition - cameraPos)));
  }

  fragColor = vec4(col, 1.0);
}

void main()
{
  vec2 pos = surf.wPos / 2.0 + vec2(0.5);
  pos.y = 1.0 - pos.y;
  pos = pos * vec2(iResolution);

  vec4 fragColor = vec4(0.0);
  mainImage(fragColor, pos);

  out_fragColor = fragColor;
}
