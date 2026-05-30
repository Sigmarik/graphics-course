#pragma once
#include "spaghetti_renderer/spaghetti.hpp"
#include "spaghetti_renderer/primitives/FragmentOnlyShader.hpp"
#include "spaghetti_renderer/primitives/Texture.hpp"

class Water
{
public:
  void init(spg::App& app, vk::Format depthFormat);

  void render(spg::App& app, float waterLevel, spg::Texture& color, spg::Texture& depth, spg::Texture& reflection);

  spg::Texture& getTexture() { return sceneWithWater; }
  spg::Texture& getDepth() { return newDepth; }

private:
  spg::FragmentOnlyShader rayTracedWaterShader{};

  spg::Texture sceneWithWater{};
  spg::Texture newDepth{};
};
