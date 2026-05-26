#include "VertexFragmentShader.hpp"

#include "etna/PipelineManager.hpp"
#include "etna/Profiling.hpp"

namespace spg
{
VertexFragmentShader::VertexFragmentShader(etna::GraphicsPipeline pipeline)
{
  m_inited = true;
  m_etnaPipeline = std::move(pipeline);
}

VertexFragmentShader::Dispatch::~Dispatch()
{
  if (!bindings.empty() || !attachments.empty()) etna::flush_barriers(*cmdBuf);

  if (descriptorSet != nullptr)
  {
    descriptorSet->processBarriers(*cmdBuf);
    etna::flush_barriers(*cmdBuf);
  }

  ETNA_PROFILE_GPU(*cmdBuf, renderFullscreenFragment);

  etna::RenderTargetState renderTargets(
      *cmdBuf,
      {{0, 0}, {resolutionX, resolutionY}},
      attachments,
      depthAttachment);

  cmdBuf->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getVkPipeline());

  if (!implicitVertexCnt)
  {
    assert(vertexBuf && indexBuf);
    cmdBuf->bindVertexBuffers(0, {vertexBuf}, {0});
    cmdBuf->bindIndexBuffer(indexBuf, 0, vk::IndexType::eUint32);
  }

  if (!bindings.empty())
  {
    auto programInfo = etna::get_shader_program(programName.c_str());

    etna::DescriptorSet set = etna::create_descriptor_set(
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

  if (descriptorSet != nullptr)
  {
    cmdBuf->bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline->getVkPipelineLayout(),
      1,
      {descriptorSet->getVkSet()},
      {});
  }

  if (fragmentPushConstant)
  {
    fragmentPushConstant->apply(*cmdBuf, vk::ShaderStageFlagBits::eFragment, pipeline->getVkPipelineLayout());
  }
  if (vertexPushConstant)
  {
    vertexPushConstant->apply(*cmdBuf, vk::ShaderStageFlagBits::eVertex, pipeline->getVkPipelineLayout());
  }

  if (indirectBuf)
  {
    cmdBuf->drawIndexedIndirect(
      indirectBuf->raw().get(),
      0,
      instanceCnt,
      sizeof(vk::DrawIndexedIndirectCommand));
  }
  else if (implicitVertexCnt)
  {
    cmdBuf->draw(*implicitVertexCnt, instanceCnt, 0, 0);
  }
  else
  {
    assert(geometryMapping);
    cmdBuf->drawIndexed(
      geometryMapping->indexCount,
      instanceCnt,
      geometryMapping->indexOffset,
      geometryMapping->vertexOffset,
      0);
  }
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::attach(Texture& texture)
{
  texture.prepareForShaderWrite(*cmdBuf);
  attachments.push_back({.image = texture.raw().get(), .view = texture.raw().getView({})});
  resolutionX = texture.raw().getExtent().width;
  resolutionY = texture.raw().getExtent().height;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::attach(const etna::RenderTargetState::AttachmentParams& params, glm::uvec2 resolution)
{
  attachments.push_back(params);
  resolutionX = resolution.x;
  resolutionY = resolution.y;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::attachAsDepth(Texture& texture)
{
  depthAttachment = {.image = texture.raw().get(), .view = texture.raw().getView({})};
  resolutionX = texture.raw().getExtent().width;
  resolutionY = texture.raw().getExtent().height;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::bind(unsigned id, Texture& texture, etna::Sampler& sampler)
{
  bindings.push_back(texture.getBinding(id, sampler));
  texture.prepareForShaderRead(*cmdBuf);
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::bind(unsigned id, Buffer& buffer)
{
  bindings.push_back(buffer.getBinding(id));
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::geometry(Buffer& vertices, Buffer& indices)
{
  vertexBuf = vertices.raw().get();
  indexBuf = indices.raw().get();
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::geometry(vk::Buffer vertices, vk::Buffer indices)
{
  vertexBuf = vertices;
  indexBuf = indices;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::geomMapping(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset)
{
  geometryMapping = {
    .indexCount = indexCount,
    .indexOffset = indexOffset,
    .vertexOffset = vertexOffset,
  };
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::implicitVertexCount(uint32_t implicitVertexCount)
{
  implicitVertexCnt = implicitVertexCount;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::instanceCount(uint32_t count)
{
  instanceCnt = count;
  return *this;
}

VertexFragmentShader::Dispatch& VertexFragmentShader::Dispatch::indirect(Buffer& commands, uint32_t drawCount)
{
  indirectBuf = &commands;
  instanceCnt = drawCount;
  return *this;
}

void VertexFragmentShader::init()
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sProgramIdx = 0;
  ++sProgramIdx;

  m_programName = m_programName + '#' + std::to_string(sProgramIdx);

  etna::create_program(m_programName.c_str(),
    {m_fragmentShaderPath, m_vertexShaderPath});

  auto& pipelineManager = etna::get_context().getPipelineManager();

  m_etnaPipeline = {};
  m_etnaPipeline =
    pipelineManager.createGraphicsPipeline(
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

VertexFragmentShader& VertexFragmentShader::vertexFormat(const etna::VertexByteStreamFormatDescription& format)
{
  m_creationInfo.vertexShaderInput = {
    .bindings = {etna::VertexShaderInputDescription::Binding{
      .byteStreamDescription = format,
    }},
  };
  return *this;
}

}
