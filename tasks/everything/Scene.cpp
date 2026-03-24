#include "Scene.h"

#include "imgui.h"

void Scene::initialize()
{
  deferred.resolution(getResolution()).init(&getCmdBuf());
  bindless.init(*this);

  ssao.init(*this);

  aliasedScene.name("preFXAA")
    .size(getResolution())
    .useColorAttachment()
    .format(vk::Format::eB8G8R8A8Unorm)
    .useSampled()
    .init(&getCmdBuf());

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

  ssao.render(*this, deferred.depth, deferred.normalEmissive);

  lightMixer.dispatch(getCmdBuf())
    .bind(0, deferred.albedo, getDefaultSampler())
    .bind(1, ssao.getAo(), getDefaultSampler())
    .attach(aliasedScene);

  fxaaShader.dispatch(getCmdBuf())
    .bind(0, aliasedScene, getDefaultSampler())
    .attach(getScreenAttachment(), getResolution());
}

void Scene::renderGui()
{
}
