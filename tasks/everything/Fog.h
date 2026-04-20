#pragma once

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/Texture.hpp>

#include <spaghetti_renderer/spaghetti.hpp>

#include "ShadowMap.h"

class Fog
{
public:
  unsigned quality = 30;

  void init(spg::App& app, ShadowMap& shadowMap);

  void render(spg::App& app, const ShadowMap& shadows, spg::Texture& depth);
  spg::Texture& getTexture() { return m_output; }

private:
  spg::FragmentOnlyShader m_fogShader{};
  spg::Texture m_output{};
};
