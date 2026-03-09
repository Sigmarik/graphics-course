#pragma once

#include "etna/GraphicsPipeline.hpp"

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
}
