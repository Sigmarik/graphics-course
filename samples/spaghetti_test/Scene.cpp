#include "Scene.h"

void Scene::initialize()
{
  colorBuffer.size(sizeof(glm::vec3));
  colorBuffer.init();

  colorBuffer.copyFrom(glm::vec3(0, 1, 0));

  fullRed.shaderPath(SPAGHETTI_TEST_SHADERS_ROOT "full_red.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm);
  fullRed.init();
}

void Scene::render()
{
  fullRed.dispatch(getCmdBuf())
    .bind(0, colorBuffer)
    .attach(getScreenAttachment(), getResolution());
}
