#pragma once

#include <optional>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

// Am I too lazy to make an actually good architecture for model processing? Yes.
// Is it because I was the one to implement the exact same module for the CharmQuarkEngine? Yes.
struct Vertex
{
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 texCoords{};
  glm::vec3 tangent{};
};

struct Model
{
  static std::optional<Model> fromGltf(const std::string& path);

  void toBin(const std::string& path) const;
  void toGltf(const std::string& path) const;

  using index_t = unsigned int;

  std::vector<Vertex> vertices{};
  std::vector<index_t> indices{};

  void append(const Model& model);
  void append(const Model& model, const glm::mat4& matrix);
};
