#pragma once

#include "BindlessScene.h"
#include "Fog.h"
#include "SSAO.h"
#include "SSSS.h"
#include "ShadowMap.h"

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

  spg::FragmentOnlyShader directLightingShader{};

  Fog fog{};

  SSAO ssao{};

  spg::Texture skySphere{};
  spg::Texture skySphereBlurry{};

  ShadowMap shadowMap{};
  spg::FragmentOnlyShader lightMixer{};
  spg::Texture aliasedScene{};
  spg::Texture directLight{};

  spg::FragmentOnlyShader fxaaShader{};
  SSSS subsurface{};

  bool enableFXAA = true;
  bool enableSSAO = true;
  bool enableSSSS = false;
};
