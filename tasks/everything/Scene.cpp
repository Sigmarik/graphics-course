#include "Scene.h"

void Scene::initialize()
{
  deferred.resolution(getResolution()).init(&getCmdBuf());
  bindless.init(*this);

  lightMixer
    .shaderPath(EVERYTHING_SHADERS_ROOT "/combine_lighting.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm)
    .init();
}

void Scene::render()
{
  bindless.render(*this, deferred);
  lightMixer.dispatch(getCmdBuf())
    .bind(0, deferred.albedo, getDefaultSampler())
    .bind(1, deferred.normalEmissive, getDefaultSampler())
    .attach(getScreenAttachment(), getResolution());
}
