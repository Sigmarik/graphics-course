#pragma once

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>
#include <spaghetti_renderer/primitives/Texture.hpp>

#include <spaghetti_renderer/spaghetti.hpp>

class ShadowMap
{
public:
  void init(spg::App& app, const glm::vec3& light_direction = glm::vec3(1.0, -1.0, 1.0));
  void updateCameraPositions(const glm::vec3& main_camera_pos);

  Camera& getCamera(unsigned level) { return m_cameras[level]; }
  spg::Texture& getTexture(unsigned level) { return m_textures[level]; }
  const Camera& getCamera(unsigned level) const { return m_cameras[level]; }
  const spg::Texture& getTexture(unsigned level) const { return m_textures[level]; }

  static constexpr unsigned NUM_LEVELS = 4;
  static constexpr unsigned MAP_RESOLUTION = 1024;

private:
  std::array<Camera, NUM_LEVELS> m_cameras{};
  std::array<spg::Texture, NUM_LEVELS> m_textures{};
};
