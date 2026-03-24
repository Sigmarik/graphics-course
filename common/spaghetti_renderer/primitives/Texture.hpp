#pragma once

#include <optional>

#include <etna/Image.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Sampler.hpp>
#include <glm/vec2.hpp>


namespace spg
{
class Texture
{
public:
  Texture() = default;
  explicit Texture(etna::Image image);
  Texture(etna::Image image, vk::ImageView fixedView);

  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&&) = default;
  Texture& operator=(Texture&&) = default;

  Texture& size(unsigned width, unsigned height)
  {
    assert(!m_inited);
    m_width = width;
    m_height = height;
    return *this;
  }

  Texture& size(glm::uvec2 size)
  {
    assert(!m_inited);
    m_width = size.x;
    m_height = size.y;
    return *this;
  }

  Texture& useDepthStencil() { assert(!m_inited); m_flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment; return *this; }
  Texture& useTransferSrc() { assert(!m_inited); m_flags |= vk::ImageUsageFlagBits::eTransferSrc; return *this; }
  Texture& useTransferDst() { assert(!m_inited); m_flags |= vk::ImageUsageFlagBits::eTransferDst; return *this; }
  Texture& useSampled() { assert(!m_inited); m_flags |= vk::ImageUsageFlagBits::eSampled; return *this; }
  Texture& useColorAttachment() { assert(!m_inited); m_flags |= vk::ImageUsageFlagBits::eColorAttachment; return *this; }

  Texture& name(const std::string& value) { assert(!m_inited); m_name = value; return *this; }
  Texture& format(vk::Format value) { assert(!m_inited); m_format = value; return *this; }

  Texture& data(const unsigned char* data);

  void init(vk::CommandBuffer* cmdBuf);

  void prepareForShaderRead(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage = vk::PipelineStageFlagBits2::eFragmentShader);
  void prepareForShaderWrite(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  etna::Image& raw() { return m_etnaImage; }

  etna::RenderTargetState::AttachmentParams getAttachment()
  {
    return {.image = raw().get(), .view = raw().getView({})};
  }

  etna::Binding getBinding(unsigned bindingId, const etna::Sampler& sampler, unsigned arrayElem = 0);

  vk::Format getFormat() { return m_inited ? m_format : m_etnaImage.getFormat(); }

private:
  unsigned m_width = 256, m_height = 256;
  vk::ImageUsageFlags m_flags;
  vk::Format m_format = vk::Format::eR8G8B8A8Srgb;
  std::string m_name = "";

  const unsigned char* m_bytes = nullptr;

  etna::Image m_etnaImage;
  std::optional<vk::ImageView> m_viewOverride;

  bool m_inited = false;
};
}
