#include "ComputeShader.hpp"

#include "etna/PipelineManager.hpp"
#include "etna/Profiling.hpp"

namespace spg
{
ComputeShader::ComputeShader(etna::ComputePipeline pipeline)
{
  m_inited = true;
  m_etnaPipeline = std::move(pipeline);
}

ComputeShader::Dispatch::~Dispatch()
{
  if (!bindings.empty()) etna::flush_barriers(*cmdBuf);

  if (descriptorSet != nullptr)
  {
    descriptorSet->processBarriers(*cmdBuf);
    etna::flush_barriers(*cmdBuf);
  }

  ETNA_PROFILE_GPU(*cmdBuf, computeDispatch);

  cmdBuf->bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->getVkPipeline());

  if (!bindings.empty())
  {
    auto programInfo = etna::get_shader_program(programName.c_str());

    etna::DescriptorSet set = etna::create_descriptor_set(
      programInfo.getDescriptorLayoutId(0),
      *cmdBuf,
      bindings);

    vk::DescriptorSet vkSet = set.getVkSet();

    cmdBuf->bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      pipeline->getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);
  }

  if (descriptorSet != nullptr)
  {
    cmdBuf->bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      pipeline->getVkPipelineLayout(),
      1,
      {descriptorSet->getVkSet()},
      {});
  }

  if (pushConstant)
  {
    pushConstant->apply(*cmdBuf, vk::ShaderStageFlagBits::eCompute, pipeline->getVkPipelineLayout());
  }

  cmdBuf->dispatch(groupCountX, groupCountY, groupCountZ);
}

ComputeShader::Dispatch& ComputeShader::Dispatch::bind(unsigned id, Texture& texture, etna::Sampler& sampler)
{
  bindings.push_back(texture.getBinding(id, sampler));
  texture.prepareForShaderRead(*cmdBuf, vk::PipelineStageFlagBits2::eComputeShader);
  return *this;
}

ComputeShader::Dispatch& ComputeShader::Dispatch::bind(unsigned id, Buffer& buffer)
{
  bindings.push_back(buffer.getBinding(id));
  return *this;
}

ComputeShader::Dispatch& ComputeShader::Dispatch::bind(unsigned id, etna::Binding binding)
{
  bindings.push_back(std::move(binding));
  // Override id if needed, actually binding object already has it
  bindings.back().binding = id;
  return *this;
}

ComputeShader::Dispatch& ComputeShader::Dispatch::groups(uint32_t x, uint32_t y, uint32_t z)
{
  groupCountX = x;
  groupCountY = y;
  groupCountZ = z;
  return *this;
}

void ComputeShader::init()
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sProgramIdx = 0;
  ++sProgramIdx;

  m_programName = m_programName + '#' + std::to_string(sProgramIdx);

  etna::create_program(m_programName.c_str(),
    {m_shaderPath});

  auto& pipelineManager = etna::get_context().getPipelineManager();

  m_etnaPipeline = {};
  m_etnaPipeline =
    pipelineManager.createComputePipeline(
      m_programName.c_str(),
      m_creationInfo);

  if (!m_bindlessBindings.empty())
  {
    auto shaderInfo = etna::get_shader_program(m_programName.c_str());

    m_descriptorSet = etna::create_persistent_descriptor_set(
      shaderInfo.getDescriptorLayoutId(1),
      std::move(m_bindlessBindings),
      true);
  }
}

}


