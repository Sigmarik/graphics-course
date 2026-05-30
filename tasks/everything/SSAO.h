#pragma once

#include "BindlessScene.h"
#include "scene/SceneManager.hpp"

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>


#include <spaghetti_renderer/spaghetti.hpp>

class SSAO
{
public:
  void init(spg::App& app);
  void render(spg::App& app, spg::Texture& depth, spg::Texture& normal);

  spg::Texture& getAo() { return ao; }

private:
  spg::Texture noisyAo{};
  spg::Texture ao{};

  spg::Buffer kernelVectors{};

  spg::FragmentOnlyShader ssaoShader{};
  spg::FragmentOnlyShader spatialDenoise{};
};
