#include "Scene.h"

struct ShaderToyParams
{
  uint32_t resolutionX = 0;
  uint32_t resolutionY = 0;
  float mouseX = 0;
  float mouseY = 0;
  float time = 0;
};

void Scene::initialize()
{
  ballTexture.name("ball texture")
    .useColorAttachment().useSampled();
  ballTexture.init();

  intermediate.shaderPath(SPAGHETTI_TEST_SHADERS_ROOT "intermediate.frag.spv")
    .addColorAttachment(ballTexture.getFormat());
  intermediate.init();

  toy.shaderPath(SPAGHETTI_TEST_SHADERS_ROOT "toy.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm);
  toy.init();
}

void Scene::render()
{
  ShaderToyParams params;

  params.resolutionX = getResolution().x;
  params.resolutionY = getResolution().y;
  params.mouseX = 0;
  params.mouseY = 0;

  intermediate.dispatch(getCmdBuf())
    .push(params)
    .attach(ballTexture);

  toy.dispatch(getCmdBuf())
    .bind(0, ballTexture, getDefaultSampler())
    .bind(1, ballTexture, getDefaultSampler())
    .attach(getScreenAttachment(), getResolution());
}
