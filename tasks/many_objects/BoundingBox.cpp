#include "BoundingBox.hpp"

void BoundingBox::stretch(const glm::vec3& point)
{
  if (hasSamples)
  {
    min.x = glm::min(min.x, point.x);
    min.y = glm::min(min.y, point.y);
    min.z = glm::min(min.z, point.z);
    max.x = glm::max(max.x, point.x);
    max.y = glm::max(max.y, point.y);
    max.z = glm::max(max.z, point.z);
  }
  else
  {
    min = max = point;
    hasSamples = true;
  }
}

static glm::vec3 transformed(const glm::mat4& transform, const glm::vec3& vector)
{
  glm::vec4 result = transform * glm::vec4(vector, 1.0);
  result /= result.w;
  return xyz(result);
}

BoundingBox BoundingBox::transform(const glm::mat4& matrix) const
{
  BoundingBox box;
  // TODO: This thing is slow as f... Spheres would have probably been a better option.
  box.stretch(transformed(matrix, glm::vec3(min.x, min.y, min.z)));
  box.stretch(transformed(matrix, glm::vec3(min.x, min.y, max.z)));
  box.stretch(transformed(matrix, glm::vec3(min.x, max.y, min.z)));
  box.stretch(transformed(matrix, glm::vec3(min.x, max.y, max.z)));
  box.stretch(transformed(matrix, glm::vec3(max.x, min.y, min.z)));
  box.stretch(transformed(matrix, glm::vec3(max.x, min.y, max.z)));
  box.stretch(transformed(matrix, glm::vec3(max.x, max.y, min.z)));
  box.stretch(transformed(matrix, glm::vec3(max.x, max.y, max.z)));
  return box;
}

bool BoundingBox::shouldRender() const
{
  if (min.x > 1.0 || min.y > 1.0 || min.z > 1.0)
  {
    return false;
  }
  if (max.x < -1.0 || max.y < -1.0 || max.z < -1.0)
  {
    return false;
  }
  return true;
}
