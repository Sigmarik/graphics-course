#include "Buffer.hpp"

#include "etna/Etna.hpp"

#include <etna/GlobalContext.hpp>

namespace spg
{

Buffer::Buffer(etna::Buffer buffer)
{
  m_etnaBuffer = std::move(buffer);
  m_inited = true;
}

void Buffer::init()
{
  assert(!m_inited);
  m_inited = true;

  static unsigned sBufferIdx = 0;
  ++sBufferIdx;

  auto& ctx = etna::get_context();

  std::string name = m_name + '#' + std::to_string(sBufferIdx);
  m_etnaBuffer = ctx.createBuffer(etna::Buffer::CreateInfo{
    .size = m_size,
    .bufferUsage = m_usage,
    .memoryUsage = m_memUsage,
    .name = name,
  });

  m_etnaBuffer.map();
}

void Buffer::prepareForShaderRead(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage)
{
  etna::set_state(
    cmd_buf,
    raw().get(),
    stage,
    vk::AccessFlagBits2::eShaderRead);
}

void Buffer::prepareForShaderWrite(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage)
{
  etna::set_state(
    cmd_buf,
    raw().get(),
    stage,
    vk::AccessFlagBits2::eShaderWrite);
}

}
