#include "SSAO.h"

static float randomFloat(float min, float max)
{
  return rand() / static_cast<float>(RAND_MAX) * (max - min) + min;
}

static glm::vec3 randomPositiveZ()
{
  glm::vec3 vec(10);
  while (glm::length(vec) > 1.0)
  {
    vec.x = randomFloat(-1.0f, 1.0f);
    vec.y = randomFloat(-1.0f, 1.0f);
    vec.z = randomFloat(-0.0f, 1.0f);
  }
  return vec;
}

void SSAO::init(spg::App& app)
{
  ao.name("ambientOcclusion")
    .size(app.getResolution().x, app.getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&app.getCmdBuf());

  noisyAo.name("noisyAmbientOcclusion")
    .size(app.getResolution().x, app.getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&app.getCmdBuf());

  ssaoShader.programName("SSAO")
    .shaderPath(EVERYTHING_SHADERS_ROOT "/ssao.frag.spv")
    .addColorAttachment(noisyAo.getFormat())
    .init();

  spatialDenoise.programName("SpatialDenoise")
    .shaderPath(EVERYTHING_SHADERS_ROOT "/spatial_denoise.frag.spv")
    .addColorAttachment(ao.getFormat())
    .init();

  std::vector<glm::vec4> kernelVecs(64);

  std::ignore = randomPositiveZ();
  for (unsigned id = 0; id < kernelVecs.size(); id++)
  {
    kernelVecs[id] = glm::vec4(randomPositiveZ(), 0.0);
  }

  kernelVectors.useStorage().initAndCopy(kernelVecs);
}

void SSAO::render(spg::App& app, spg::Texture& depth, spg::Texture& normal)
{
  struct Matrices
  {
    glm::mat4 proj;
    glm::mat4 invProj;
  };

  Matrices matrices;
  matrices.invProj = app.getWorldInvProj();
  matrices.proj = glm::inverse(matrices.invProj);

  ssaoShader.dispatch(app.getCmdBuf())
    .bind(0, depth, app.getDefaultSampler())
    .bind(1, normal, app.getDefaultSampler())
    .bind(10, kernelVectors)
    .push(matrices)
    .attach(noisyAo);

  spatialDenoise.dispatch(app.getCmdBuf())
    .bind(0, noisyAo, app.getDefaultSampler())
    .bind(1, depth, app.getDefaultSampler())
    .bind(2, normal, app.getDefaultSampler())
    .attach(ao);
}
