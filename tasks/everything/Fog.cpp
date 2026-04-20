#include "Fog.h"


void Fog::init(spg::App& app, ShadowMap& shadowMap)
{
  for (unsigned idx = 0; idx < shadowMap.NUM_LEVELS; ++idx)
  {
    m_fogShader.addPersistentBinding(shadowMap.getTexture(idx), app.getDefaultSampler());
  }

  m_output.name("fogOutput")
    .size(app.getResolution().x, app.getResolution().y)
    .format(vk::Format::eR8G8B8A8Unorm)
    .useColorAttachment()
    .useSampled()
    .init(&app.getCmdBuf());

  m_fogShader.programName("fogShader")
    .shaderPath(EVERYTHING_SHADERS_ROOT "/fog.frag.spv")
    .addColorAttachment(m_output.getFormat())
    .init();
}

void Fog::render(spg::App& app, const ShadowMap& shadows, spg::Texture& depth)
{
  struct CombinedMatrices
  {
    glm::mat4 invProjView;
    glm::mat4 lightProjView;
    glm::mat4 invView;
    glm::mat4 lightInvView;
  };
  CombinedMatrices combinedMatrices;
  const Camera& shadowCam = shadows.getCamera(0);
  combinedMatrices.invProjView = glm::inverse(app.getWorldViewProj());
  combinedMatrices.lightProjView = shadowCam.projTm(1.0f) * shadowCam.viewTm();
  combinedMatrices.invView = glm::inverse(app.getWorldView());
  combinedMatrices.lightInvView = glm::inverse(shadowCam.viewTm());

  m_fogShader.dispatch(app.getCmdBuf())
    .bind(0, depth, app.getDefaultSampler())
    .push(combinedMatrices)
    .attach(m_output);
}
