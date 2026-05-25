#include "SSSS.h"

void SSSS::init(spg::App& app)
{
  blurryDirect.name("bluredDirect")
    .size(app.getResolution().x, app.getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&app.getCmdBuf());

  spatialDenoise.programName("Subsurface")
    .shaderPath(EVERYTHING_SHADERS_ROOT "/subsurface.frag.spv")
    .addColorAttachment(blurryDirect.getFormat())
    .init();
}

void SSSS::render(spg::App& app, spg::Texture& direct, spg::Texture& depth, spg::Texture& normal)
{
  struct Matrices
  {
    glm::mat4 proj;
    glm::mat4 invProj;
  };

  Matrices matrices;
  matrices.invProj = app.getWorldInvProj();
  matrices.proj = glm::inverse(matrices.invProj);

  spatialDenoise.dispatch(app.getCmdBuf())
    .push(matrices)
    .bind(0, direct, app.getDefaultSampler())
    .bind(1, depth, app.getDefaultSampler())
    .bind(2, normal, app.getDefaultSampler())
    .attach(blurryDirect);
}
