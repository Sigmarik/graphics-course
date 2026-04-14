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

  shadowMap.name("shadowMap")
    .size(glm::uvec2(1024, 1024))
    .format(vk::Format::eD32Sfloat)
    .useDepthStencil()
    .useSampled()
    .init(&getCmdBuf());

  directLight.name("directLighting")
    .size(getResolution().x, getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  directLightingShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/direct_light.frag.spv")
    .addColorAttachment(directLight.getFormat())
    .init();

  lightMixer
    .shaderPath(EVERYTHING_SHADERS_ROOT "/combine_lighting.frag.spv")
    .addColorAttachment(aliasedScene.getFormat())
    .init();

  fxaaShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/fxaa.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm)
    .init();

  shadowCamera.lookAt(glm::vec3(100, 100, 100), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  shadowCamera.orthographic = true;
}

void Scene::render()
{
  bindless.render(*this, deferred);

  glm::vec3 shadowCameraPosition = getCam().position - shadowCamera.forward() * 500.0f;
  shadowCamera.setPixelAccuratePosition(shadowCameraPosition, glm::uvec2(1024, 1024));

  overrideCamera(shadowCamera);
  bindless.renderShadowMap(*this, shadowCamera, shadowMap);
  clearCameraOverride();

  ssao.render(*this, deferred.depth, deferred.normalEmissive);

  struct CombinedMatrices
  {
    glm::mat4 invProjView;
    glm::mat4 lightProjView;
    glm::mat4 invView;
    glm::mat4 lightInvView;
  };
  CombinedMatrices combinedMatrices;
  combinedMatrices.invProjView = glm::inverse(getWorldViewProj());
  combinedMatrices.lightProjView = shadowCamera.projTm(1.0f) * shadowCamera.viewTm();
  combinedMatrices.invView = glm::inverse(getWorldView());
  combinedMatrices.lightInvView = glm::inverse(shadowCamera.viewTm());
  directLightingShader.dispatch(getCmdBuf())
    .bind(0, deferred.depth, getDefaultSampler())
    .bind(1, deferred.normalEmissive, getDefaultSampler())
    .bind(2, shadowMap, getDefaultSampler())
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
