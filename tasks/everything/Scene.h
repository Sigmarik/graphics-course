#pragma once

#include "BindlessScene.h"
#include "Fog.h"
#include "SSAO.h"
#include "SSSS.h"
#include "ShadowMap.h"
#include "Particles.h"
#include "Water.h"

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
  DeferredTextureBunch reflectionDeferred{};

  spg::FragmentOnlyShader directLightingShader{};

  Fog fog{};

  SSAO ssao{};

  spg::Texture skySphere{};
  spg::Texture skySphereBlurry{};

  ShadowMap shadowMap{};
  spg::FragmentOnlyShader lightMixer{};
  spg::Texture aliasedScene{};
  spg::Texture directLight{};
  spg::Texture reflectionDirectLight{};

  spg::Texture reflectionWithLighting{};

  spg::Texture fullWhite{};
  spg::Texture fullBlack{};

  spg::FragmentOnlyShader fxaaShader{};
  SSSS subsurface{};
  Particles particles{};
  Water water{};

  float waterLevel = 1.0f;

  bool enableFXAA = true;
  bool enableSSAO = true;
  bool enableSSSS = false;
  bool enableParticles = true;
  bool enableWater = true;
};
