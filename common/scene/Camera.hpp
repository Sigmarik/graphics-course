#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>


struct Camera
{
  glm::vec3 position;
  glm::quat rotation;
  float fov{60};
  float zNear{0.01f};
  float zFar{1000};
  bool orthographic = false;
  float orthoHeight = 50.0f;

  void lookAt(glm::vec3 from, glm::vec3 to, glm::vec3 up)
  {
    position = from;
    rotation = glm::quatLookAtLH(normalize(to - from), normalize(up));
  }

  void rotate(float up_angle_deg, float right_angle_deg)
  {
    glm::quat yaw = glm::angleAxis(glm::radians(right_angle_deg), glm::vec3{0, -1, 0});
    glm::quat pitch = glm::angleAxis(glm::radians(up_angle_deg), glm::vec3{1, 0, 0});
    rotation = yaw * rotation * pitch;
  }

  void move(glm::vec3 offset) { position += offset; }

  const glm::vec3 right() const { return rotation * glm::vec3{-1, 0, 0}; }

  const glm::vec3 up() const { return rotation * glm::vec3{0, 1, 0}; }

  const glm::vec3 forward() const { return rotation * glm::vec3{0, 0, 1}; }

  glm::mat4x4 viewItm() const
  {
    return translate(glm::identity<glm::mat4>(), position) * mat4_cast(rotation);
  }

  glm::mat4x4 viewTm() const { return inverse(viewItm()); }

  glm::mat4x4 projTm(float aspect) const
  {
    float halfWidth = aspect * orthoHeight * 0.5f;
    float halfHeight = orthoHeight * 0.5f;
    return orthographic ? glm::orthoLH_ZO(halfWidth, -halfWidth,
      halfHeight, -halfHeight, zNear, zFar)
      : glm::perspectiveLH_ZO(-glm::radians(fov), aspect, zNear, zFar);
  }

  void setPixelAccuratePosition(glm::vec3 desiredPos, glm::uvec2 resolution)
  {
    glm::mat4 viewMatrix = viewTm();
    glm::mat4 projMatrix = projTm(float(resolution.x) / float(resolution.y));
    glm::mat4 viewProjMatrix = projMatrix * viewMatrix;
    glm::mat4 invViewProjMatrix = glm::inverse(viewProjMatrix);

    glm::vec2 fResolution = glm::vec2(resolution.x, resolution.y) * 0.5f;

    glm::vec4 screenSpaceDesiredPos4 = viewProjMatrix * glm::vec4(desiredPos, 1.0f);
    glm::vec2 screenSpacePosRounded = glm::vec2(screenSpaceDesiredPos4) / screenSpaceDesiredPos4.w *
      fResolution;
    screenSpacePosRounded = glm::round(screenSpacePosRounded);
    screenSpacePosRounded /= fResolution;
    glm::vec2 screenSpaceShift = screenSpacePosRounded - glm::vec2(screenSpaceDesiredPos4);
    glm::vec4 roundingShift4 = invViewProjMatrix * glm::vec4(screenSpaceShift, 0.0f, 0.0f);
    glm::vec3 roundingShift = glm::vec3(roundingShift4);
    position = desiredPos + roundingShift;
  }
};
