#include "Scene.h"

#include "imgui.h"

void Scene::initialize()
{
  deferred.resolution(getResolution()).init(&getCmdBuf());
  bindless.init(*this);

  ssao.init(*this);

  aliasedScene.name("preFXAA")
    .size(getResolution())
    .format(vk::Format::eB8G8R8A8Unorm)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  directLight.name("directLighting")
    .size(getResolution().x, getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  shadowMap.init(*this);

  directLightingShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/direct_light.frag.spv")
    .addColorAttachment(directLight.getFormat());
  for (unsigned smapIdx = 0; smapIdx < ShadowMap::NUM_LEVELS; ++smapIdx)
  {
    directLightingShader.addPersistentBinding(shadowMap.getTexture(smapIdx), getDefaultSampler());
  }
  directLightingShader.init();

  lightMixer
    .shaderPath(EVERYTHING_SHADERS_ROOT "/combine_lighting.frag.spv")
    .addColorAttachment(aliasedScene.getFormat())
    .init();

  fxaaShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/fxaa.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm)
    .init();
}

void Scene::render()
{
  bindless.render(*this, deferred);

  shadowMap.updateCameraPositions(getCam().position);
  bindless.renderShadowMap(*this, shadowMap);

  ssao.render(*this, deferred.depth, deferred.normalEmissive);

  struct CombinedMatrices
  {
    glm::mat4 invProjView;
    glm::mat4 lightProjView;
    glm::mat4 invView;
    glm::mat4 lightInvView;
  };
  CombinedMatrices combinedMatrices;
  Camera& shadowCam = shadowMap.getCamera(0);
  combinedMatrices.invProjView = glm::inverse(getWorldViewProj());
  combinedMatrices.lightProjView = shadowCam.projTm(1.0f) * shadowCam.viewTm();
  combinedMatrices.invView = glm::inverse(getWorldView());
  combinedMatrices.lightInvView = glm::inverse(shadowCam.viewTm());
  directLightingShader.dispatch(getCmdBuf())
    .bind(0, deferred.depth, getDefaultSampler())
    .bind(1, deferred.normalEmissive, getDefaultSampler())
    .push(combinedMatrices)
    .attach(directLight);

  lightMixer.dispatch(getCmdBuf())
    .bind(0, deferred.albedo, getDefaultSampler())
    .bind(1, ssao.getAo(), getDefaultSampler())
    .bind(2, directLight, getDefaultSampler())
    .attach(aliasedScene);

  fxaaShader.dispatch(getCmdBuf())
    .bind(0, aliasedScene, getDefaultSampler())
    .attach(getScreenAttachment(), getResolution());
}

void Scene::renderGui()
{
}
