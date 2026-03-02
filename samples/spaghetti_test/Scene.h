#pragma once

#include "scene/SceneManager.hpp"

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>


#include <spaghetti_renderer/spaghetti.hpp>

class Scene : public spg::App
{
protected:
  void initialize() override;
  void render() override;

private:
  std::vector<spg::Texture> textures;
  spg::Buffer instanceInfo;
  spg::Buffer indirect;

  spg::Texture depth;
  spg::VertexFragmentShader shader;

  uint32_t indirectCount = 0;

  SceneManager sceneManager;
};
