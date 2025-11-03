#pragma once

#include <glm/glm.hpp>

class BoundingBox
{
public:
  glm::vec3 min{};
  glm::vec3 max{};

  BoundingBox() = default;
  BoundingBox(const glm::vec3& min, const glm::vec3& max)
    : min(min)
    , max(max)
  {
  }

  void stretch(const glm::vec3& point);
  BoundingBox transform(const glm::mat4& matrix) const;

  bool shouldRender() const;

private:
  bool hasSamples = false;
};
