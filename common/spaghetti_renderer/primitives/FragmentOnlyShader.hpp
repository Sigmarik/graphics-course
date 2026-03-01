#pragma once

#include <filesystem>

#include "etna/GraphicsPipeline.hpp"

#include "Texture.hpp"
#include "Buffer.hpp"

#include <glm/vec2.hpp>

namespace spg
{

struct GenericPushConstant
{
  virtual ~GenericPushConstant() = default;
  virtual void apply(vk::CommandBuffer& cmd_buf, vk::ShaderStageFlagBits stage, vk::PipelineLayout layout) = 0;
};

template <class T>
struct PushConstant : public GenericPushConstant
{
  PushConstant(const T& content) : m_content(content) {}

  void apply(vk::CommandBuffer& cmd_buf, vk::ShaderStageFlagBits stage, vk::PipelineLayout layout) override
  {
    cmd_buf.pushConstants<T>(layout, stage, 0, { m_content });
  }

  T m_content;
};

class FragmentOnlyShader
{
public:
  FragmentOnlyShader() = default;
  explicit FragmentOnlyShader(etna::GraphicsPipeline pipeline);

  FragmentOnlyShader(const FragmentOnlyShader&) = delete;
  FragmentOnlyShader& operator=(const FragmentOnlyShader&) = delete;
  FragmentOnlyShader(FragmentOnlyShader&&) = default;
  FragmentOnlyShader& operator=(FragmentOnlyShader&&) = default;

  etna::GraphicsPipeline& raw() { return m_etnaPipeline; }

  struct Dispatch
  {
    friend class FragmentOnlyShader;

  private:
    Dispatch(vk::CommandBuffer& buffer, etna::GraphicsPipeline& pipeline)
      : cmdBuf(&buffer), pipeline(&pipeline) {}

  public:
    ~Dispatch();

    Dispatch(const Dispatch&) = delete;
    Dispatch& operator=(const Dispatch&) = delete;
    Dispatch(Dispatch&&) = default;
    Dispatch& operator=(Dispatch&&) = default;

    Dispatch& attach(Texture& texture);
    Dispatch& attach(const etna::RenderTargetState::AttachmentParams& params, glm::uvec2 resolution);
    Dispatch& attachAsDepth(Texture& texture);
    Dispatch& bind(unsigned id, Texture& texture, etna::Sampler& sampler);
    Dispatch& bind(unsigned id, Buffer& buffer);

    template <class T>
    Dispatch& push(const T& constant)
    {
      pushConstant = std::unique_ptr<GenericPushConstant>(new PushConstant<T>(constant));
      return *this;
    }

  private:
    vk::CommandBuffer* cmdBuf;
    etna::GraphicsPipeline* pipeline;
    unsigned resolutionX = 0, resolutionY = 0;
    std::vector<etna::Binding> bindings;
    std::vector<etna::RenderTargetState::AttachmentParams> attachments;
    etna::RenderTargetState::AttachmentParams depthAttachment;
    std::unique_ptr<GenericPushConstant> pushConstant;

    std::string programName;
  };

  void init();

  Dispatch dispatch(vk::CommandBuffer& commandBuffer)
  {
    assert(m_inited);
    Dispatch dispatch(commandBuffer, m_etnaPipeline);
    dispatch.programName = m_programName;
    return dispatch;
  }

  FragmentOnlyShader& shaderPath(const std::filesystem::path& value)
  {
    assert(!m_inited);
    m_fragmentShaderPath = value;
    return *this;
  }

  FragmentOnlyShader& programName(const std::string& value)
  {
    assert(!m_inited);
    m_programName = value;
    return *this;
  }

  FragmentOnlyShader& depthOutputFormat(Texture& texture)
  {
    return depthOutputFormat(texture.raw().getFormat());
  }

  FragmentOnlyShader& depthOutputFormat(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.depthAttachmentFormat = format;
    return *this;
  }

  FragmentOnlyShader& stencilOutputFormat(Texture& texture)
  {
    return stencilOutputFormat(texture.raw().getFormat());
  }

  FragmentOnlyShader& stencilOutputFormat(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.stencilAttachmentFormat = format;
    return *this;
  }

  FragmentOnlyShader& addColorAttachment(Texture& texture)
  {
    return addColorAttachment(texture.raw().getFormat());
  }

  FragmentOnlyShader& addColorAttachment(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.colorAttachmentFormats.push_back(format);
    if (m_creationInfo.blendingConfig.attachments.size() < m_creationInfo.fragmentShaderOutput.colorAttachmentFormats.size())
    {
      m_creationInfo.blendingConfig.attachments.push_back({
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
      });
    }
    return *this;
  }

private:
  bool m_inited = false;

  std::filesystem::path m_fragmentShaderPath = "";
  std::string m_programName = "";
  etna::GraphicsPipeline::CreateInfo m_creationInfo;

  etna::GraphicsPipeline m_etnaPipeline;
};

}
