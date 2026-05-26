#pragma once

#include <filesystem>
#include <unordered_map>
#include <optional>

#include "etna/GraphicsPipeline.hpp"

#include "Texture.hpp"
#include "Buffer.hpp"
#include "PushConstant.hpp"

#include <glm/vec2.hpp>

namespace spg
{
class VertexFragmentShader
{
public:
  VertexFragmentShader() = default;
  explicit VertexFragmentShader(etna::GraphicsPipeline pipeline);

  VertexFragmentShader(const VertexFragmentShader&) = delete;
  VertexFragmentShader& operator=(const VertexFragmentShader&) = delete;
  VertexFragmentShader(VertexFragmentShader&&) = default;
  VertexFragmentShader& operator=(VertexFragmentShader&&) = default;

  etna::GraphicsPipeline& raw() { return m_etnaPipeline; }

  struct Dispatch
  {
    friend class VertexFragmentShader;

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
    Dispatch& attach(Texture& texture, vk::AttachmentLoadOp loadOp);
    Dispatch& attach(const etna::RenderTargetState::AttachmentParams& params, glm::uvec2 resolution);
    Dispatch& attachAsDepth(Texture& texture);
    Dispatch& attachAsDepth(Texture& texture, vk::AttachmentLoadOp loadOp);
    Dispatch& bind(unsigned id, Texture& texture, etna::Sampler& sampler);
    Dispatch& bind(unsigned id, Buffer& buffer);

    Dispatch& geometry(Buffer& vertices, Buffer& indices);
    Dispatch& geometry(vk::Buffer vertices, vk::Buffer indices);
    Dispatch& geomMapping(uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0);
    Dispatch& implicitVertexCount(uint32_t implicitVertexCount);
    Dispatch& instanceCount(uint32_t count);
    Dispatch& indirect(Buffer& commands, uint32_t drawCount);

    Dispatch& primitiveTopology(vk::PrimitiveTopology topology);

    template <class T>
    Dispatch& pushFragment(const T& constant)
    {
      fragmentPushConstant = std::unique_ptr<GenericPushConstant>(new PushConstant<T>(constant));
      return *this;
    }

    template <class T>
    Dispatch& pushVertex(const T& constant)
    {
      vertexPushConstant = std::unique_ptr<GenericPushConstant>(new PushConstant<T>(constant));
      return *this;
    }

  private:
    vk::CommandBuffer* cmdBuf = nullptr;
    etna::GraphicsPipeline* pipeline = nullptr;
    VertexFragmentShader* owner = nullptr;
    etna::PersistentDescriptorSet* descriptorSet = nullptr;

    unsigned resolutionX = 0, resolutionY = 0;
    std::vector<etna::Binding> bindings{};
    std::vector<etna::RenderTargetState::AttachmentParams> attachments{};
    etna::RenderTargetState::AttachmentParams depthAttachment;
    std::unique_ptr<GenericPushConstant> fragmentPushConstant{};
    std::unique_ptr<GenericPushConstant> vertexPushConstant{};

    vk::Buffer vertexBuf = nullptr;
    vk::Buffer indexBuf = nullptr;

    Buffer* indirectBuf = nullptr;
    uint32_t instanceCnt = 1;

    struct GeometryMapping
    {
      uint32_t indexCount = 0;
      uint32_t indexOffset = 0;
      uint32_t vertexOffset = 0;
    };

    std::optional<GeometryMapping> geometryMapping;
    std::optional<uint32_t> implicitVertexCnt;
    std::optional<vk::PrimitiveTopology> primitiveTopologyOverride;

    std::string programName;
  };

  void init();

  Dispatch dispatch(vk::CommandBuffer& command_buffer)
  {
    assert(m_inited);
    Dispatch dispatch(command_buffer, m_etnaPipeline);
    dispatch.programName = m_programName;
    dispatch.owner = this;
    if (m_descriptorSet.isValid()) dispatch.descriptorSet = &m_descriptorSet;
    return dispatch;
  }

  VertexFragmentShader& fragmentPath(const std::filesystem::path& value)
  {
    assert(!m_inited);
    m_fragmentShaderPath = value;
    return *this;
  }

  VertexFragmentShader& vertexPath(const std::filesystem::path& value)
  {
    assert(!m_inited);
    m_vertexShaderPath = value;
    return *this;
  }

  VertexFragmentShader& programName(const std::string& value)
  {
    assert(!m_inited);
    m_programName = value;
    return *this;
  }

  VertexFragmentShader& depthOutputFormat(Texture& texture)
  {
    return depthOutputFormat(texture.raw().getFormat());
  }

  VertexFragmentShader& depthOutputFormat(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.depthAttachmentFormat = format;
    return *this;
  }

  VertexFragmentShader& depthWrite(bool enable = true)
  {
    assert(!m_inited);
    m_creationInfo.depthConfig.depthWriteEnable = enable ? vk::True : vk::False;
    return *this;
  }

  VertexFragmentShader& stencilOutputFormat(Texture& texture)
  {
    return stencilOutputFormat(texture.raw().getFormat());
  }

  VertexFragmentShader& stencilOutputFormat(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.stencilAttachmentFormat = format;
    return *this;
  }

  VertexFragmentShader& addColorAttachment(Texture& texture)
  {
    return addColorAttachment(texture.raw().getFormat());
  }

  VertexFragmentShader& addColorAttachment(vk::Format format)
  {
    assert(!m_inited);
    m_creationInfo.fragmentShaderOutput.colorAttachmentFormats.push_back(format);
    if (m_creationInfo.blendingConfig.attachments.size() < m_creationInfo.fragmentShaderOutput.colorAttachmentFormats.size())
    {
      vk::PipelineColorBlendAttachmentState attachment{};

      attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

      if (m_alphaBlend)
      {
        attachment.blendEnable = vk::True;
        attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        attachment.colorBlendOp = vk::BlendOp::eAdd;

        attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        attachment.alphaBlendOp = vk::BlendOp::eAdd;
      }
      else
      {
        attachment.blendEnable = vk::False;
      }

      m_creationInfo.blendingConfig.attachments.push_back(attachment);
    }
    return *this;
  }

  VertexFragmentShader& alphaBlend(bool enable = true)
  {
    assert(!m_inited);
    m_alphaBlend = enable;
    for (auto& attachment : m_creationInfo.blendingConfig.attachments)
    {
       if (enable)
       {
         attachment.blendEnable = vk::True;
         attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
         attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
         attachment.colorBlendOp = vk::BlendOp::eAdd;

         attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
         attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
         attachment.alphaBlendOp = vk::BlendOp::eAdd;
       }
       else
       {
         attachment.blendEnable = vk::False;
       }
    }
    return *this;
  }

  VertexFragmentShader& addPersistentBinding(Texture& texture, etna::Sampler& sampler)
  {
    assert(!m_inited);
    unsigned arrayElem = static_cast<unsigned>(m_bindlessBindings.size());
    m_bindlessBindings.emplace_back(texture.getBinding(0, sampler, arrayElem));
    return *this;
  }

  VertexFragmentShader& vertexFormat(const etna::VertexByteStreamFormatDescription& format);

private:
  etna::GraphicsPipeline& getPipelineForTopology(vk::PrimitiveTopology topology);

  bool m_inited = false;

  std::filesystem::path m_fragmentShaderPath = "";
  std::filesystem::path m_vertexShaderPath = "";
  std::string m_programName = "";
  bool m_alphaBlend = false;
  etna::GraphicsPipeline::CreateInfo m_creationInfo;

  etna::GraphicsPipeline m_etnaPipeline;
  std::unordered_map<int, etna::GraphicsPipeline> m_pipelineByTopology;

  etna::PersistentDescriptorSet m_descriptorSet;
  std::vector<etna::Binding> m_bindlessBindings;
};

}
