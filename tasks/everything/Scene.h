#pragma once

#include "BindlessScene.h"
#include "SSAO.h"
#include "scene/SceneManager.hpp"

#include <spaghetti_renderer/primitives/FragmentOnlyShader.hpp>
#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>


#include <spaghetti_renderer/spaghetti.hpp>

class Scene : public spg::App
{
protected:
  void initialize() override;
  void render() override;

  void renderGui() override;

private:
  BindlessScene bindless{};
  DeferredTextureBunch deferred{};

  SSAO ssao{};

  spg::FragmentOnlyShader lightMixer{};
  spg::Texture aliasedScene{};

  spg::FragmentOnlyShader fxaaShader{};
};
