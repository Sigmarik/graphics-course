#include "FragmentOnlyShader.hpp"

#include "etna/PipelineManager.hpp"
#include "etna/Profiling.hpp"

namespace spg
{
FragmentOnlyShader::FragmentOnlyShader(etna::GraphicsPipeline pipeline)
{
  m_inited = true;
  m_etnaPipeline = std::move(pipeline);
}

FragmentOnlyShader::Dispatch::~Dispatch()
{
  if (!bindings.empty()) etna::flush_barriers(*cmdBuf);

  ETNA_PROFILE_GPU(*cmdBuf, renderFullscreenFragment);

  etna::RenderTargetState renderTargets(
      *cmdBuf,
      {{0, 0}, {resolutionX, resolutionY}},
      attachments,
      depthAttachment);

  cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getVkPipeline());

  if (!bindings.empty())
  {
    auto programInfo = etna::get_shader_program(programName.c_str());
    auto set = etna::create_descriptor_set(
      programInfo.getDescriptorLayoutId(0),
      *cmdBuf,
      bindings);

    vk::DescriptorSet vkSet = set.getVkSet();

    cmdBuf->bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline->getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);
  }

  if (pushConstant)
  {
    pushConstant->apply(*cmdBuf, vk::ShaderStageFlagBits::eFragment, pipeline->getVkPipelineLayout());
  }

  cmdBuf->draw(3, 1, 0, 0);
}

FragmentOnlyShader::Dispatch& FragmentOnlyShader::Dispatch::attach(Texture& texture)
{
  attachments.push_back({.image = texture.raw().get(), .view = texture.raw().getView({})});
  resolutionX = texture.raw().getExtent().width;
  resolutionY = texture.raw().getExtent().height;
  return *this;
}

FragmentOnlyShader::Dispatch& FragmentOnlyShader::Dispatch::attach(const etna::RenderTargetState::AttachmentParams& params, glm::uvec2 resolution)
{
  attachments.push_back(params);
  resolutionX = resolution.x;
  resolutionY = resolution.y;
  return *this;
}

FragmentOnlyShader::Dispatch& FragmentOnlyShader::Dispatch::attachAsDepth(Texture& texture)
{
  depthAttachment = {.image = texture.raw().get(), .view = texture.raw().getView({})};
  return *this;
}

FragmentOnlyShader::Dispatch& FragmentOnlyShader::Dispatch::bind(unsigned id, Texture& texture, etna::Sampler& sampler)
{
  bindings.push_back(texture.getBinding(id, sampler));
  return *this;
}

FragmentOnlyShader::Dispatch& FragmentOnlyShader::Dispatch::bind(unsigned id, Buffer& buffer)
{
  bindings.push_back(buffer.getBinding(id));
  return *this;
}

void FragmentOnlyShader::init()
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sProgramIdx = 0;
  ++sProgramIdx;

  m_programName = m_programName + '#' + std::to_string(sProgramIdx);

  etna::create_program(m_programName.c_str(),
    {m_fragmentShaderPath, SPAGHETTI_RENDERER_SHADERS_ROOT "fullscreen.vert.spv"});

  auto& pipelineManager = etna::get_context().getPipelineManager();

  m_etnaPipeline = {};
  m_etnaPipeline =
    pipelineManager.createGraphicsPipeline(
      m_programName.c_str(),
      m_creationInfo);
}

}
