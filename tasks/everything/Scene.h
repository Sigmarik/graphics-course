#pragma once

#include "BindlessScene.h"
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
  BindlessScene bindless{};
  DeferredTextureBunch deferred{};

  spg::FragmentOnlyShader lightMixer{};
};
