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

void Texture::init()
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sTextureUid = 0;

  std::string name = m_name + "#" + std::to_string(sTextureUid++);
  auto& ctx = etna::get_context();
  m_etnaImage = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{m_width, m_height, 1},
    .name = name,
    .format = m_format,
    .imageUsage = m_flags,
  });
}

void Texture::prepareForShaderRead(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage)
{
  assert(m_inited);

  etna::set_state(
      cmd_buf,
      raw().get(),
      stage,
      vk::AccessFlagBits2::eColorAttachmentRead,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::ImageAspectFlagBits::eColor);
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

etna::Binding Texture::getBinding(unsigned bindingId, const etna::Sampler& sampler)
{
  return {bindingId, raw().genBinding(
      sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)};
}
}
