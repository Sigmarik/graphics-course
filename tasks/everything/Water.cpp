#include "Water.h"

#include "DeferredTextureBunch.h"


void Water::init(spg::App& app, vk::Format depthFormat)
{
  sceneWithWater.name("sceneWithWater")
    .size(app.getResolution())
    .format(vk::Format::eB8G8R8A8Unorm)
    .useColorAttachment()
    .useSampled()
    .init(&app.getCmdBuf());

  newDepth
    .size(app.getResolution())
    .useDepthStencil()
    .useSampled()
    .format(DeferredTextureBunch::DEPTH_FORMAT)
    .name("depthWithWater")
    .init(&app.getCmdBuf());

  rayTracedWaterShader.programName("RayTracedWater")
    .shaderPath(EVERYTHING_SHADERS_ROOT "/ray_traced_water.frag.spv")
    .addColorAttachment(sceneWithWater.getFormat())
    .depthOutputFormat(depthFormat)
    .init();
}

void Water::render(spg::App& app, float water_level, spg::Texture& color, spg::Texture& depth, spg::Texture& reflection)
{
  struct Matrices
  {
    glm::mat4 invProjView;
    glm::mat4 projView;
    float waterLevel;
    glm::vec3 pad0;
  };

  Matrices matrices;
  matrices.invProjView = glm::inverse(app.getWorldViewProj());
  matrices.projView = app.getWorldViewProj();
  matrices.waterLevel = water_level;
  matrices.pad0 = glm::vec3(0.0f);

  rayTracedWaterShader.dispatch(app.getCmdBuf())
    .bind(0, color, app.getDefaultSampler())
    .bind(1, depth, app.getDefaultSampler())
    .bind(2, reflection, app.getDefaultSampler())
    .push(matrices)
    .attach(sceneWithWater)
    .attachAsDepth(newDepth);
}