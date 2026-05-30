#pragma once

#include "BindlessScene.h"
#include "scene/SceneManager.hpp"

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>


#include <spaghetti_renderer/spaghetti.hpp>

class SSSS
{
public:
  void init(spg::App& app);
  void render(spg::App& app, spg::Texture& direct, spg::Texture& depth, spg::Texture& normal);

  spg::Texture& getSubsurface() { return blurryDirect; }

private:
  spg::Texture blurryDirect{};
  spg::FragmentOnlyShader spatialDenoise{};
};
