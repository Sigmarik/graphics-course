#pragma once

#include "scene/SceneManager.hpp"

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>

#include <spaghetti_renderer/spaghetti.hpp>
#include "DeferredTextureBunch.h"
#include "ShadowMap.h"

class BindlessScene
{
public:
  void init(spg::App& app);
  void render(spg::App& app, DeferredTextureBunch& target);
  void renderShadowMap(spg::App& app, ShadowMap& target);

private:
  std::vector<spg::Texture> textures;
  spg::Buffer instanceInfo;
  spg::Buffer indirect;

  spg::VertexFragmentShader shader;
  spg::VertexFragmentShader depthOnlyShader;

  uint32_t indirectCount = 0;

  SceneManager sceneManager;
};
