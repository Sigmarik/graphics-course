#include "Texture.hpp"

namespace spg
{

Texture::Texture(etna::Image image)
{
  m_etnaImage = std::move(image);
  m_inited = true;
}

Texture::Texture(etna::Image image, vk::ImageView fixedView) : Texture(std::move(image))
{
  m_viewOverride = std::move(fixedView);
}

Texture& Texture::data(const unsigned char* data)
{
  m_bytes = data;
  return *this;
}

void Texture::init(vk::CommandBuffer* cmdBuf)
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sTextureUid = 0;

  std::string name = m_name + "#" + std::to_string(sTextureUid++);
  auto& ctx = etna::get_context();
  etna::Image::CreateInfo info{
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = name,
    .format = m_format,
    .imageUsage = m_flags,
  };

  if (m_bytes)
  {
    assert(cmdBuf);
    m_etnaImage = etna::create_image_from_bytes(info, *cmdBuf, m_bytes);
  }
  else
  {
    m_etnaImage = ctx.createImage(info);
  }
}

void Texture::prepareForShaderRead(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage)
{
  assert(m_inited);

  const vk::ImageAspectFlags aspectMask = raw().getAspectMaskByFormat();

  etna::set_state(
      cmd_buf,
      raw().get(),
      stage,
      vk::AccessFlagBits2::eShaderSampledRead,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      aspectMask);
}

void Texture::prepareForShaderWrite(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage)
{
  assert(m_inited);

  etna::set_state(
      cmd_buf,
      raw().get(),
      stage,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageAspectFlagBits::eColor);
}

etna::Binding Texture::getBinding(unsigned bindingId, const etna::Sampler& sampler, unsigned arrayElem)
{
  etna::Binding binding(bindingId, raw().genBinding(
      sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal));
  binding.arrayElem = arrayElem;
  return binding;
}
}
