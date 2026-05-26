#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>

#include "etna/ComputePipeline.hpp"
#include "etna/DescriptorSet.hpp"

#include "Texture.hpp"
#include "Buffer.hpp"
#include "PushConstant.hpp"

namespace spg
{
class ComputeShader
{
public:
  ComputeShader() = default;
  explicit ComputeShader(etna::ComputePipeline pipeline);

  ComputeShader(const ComputeShader&) = delete;
  ComputeShader& operator=(const ComputeShader&) = delete;
  ComputeShader(ComputeShader&&) = default;
  ComputeShader& operator=(ComputeShader&&) = default;

  etna::ComputePipeline& raw() { return m_etnaPipeline; }

  struct Dispatch
  {
    friend class ComputeShader;

  private:
    Dispatch(vk::CommandBuffer& buffer, etna::ComputePipeline& pipeline)
      : cmdBuf(&buffer), pipeline(&pipeline) {}

  public:
    ~Dispatch();

    Dispatch(const Dispatch&) = delete;
    Dispatch& operator=(const Dispatch&) = delete;
    Dispatch(Dispatch&&) = default;
    Dispatch& operator=(Dispatch&&) = default;

    Dispatch& bind(unsigned id, Texture& texture, etna::Sampler& sampler);
    Dispatch& bind(unsigned id, Buffer& buffer);
    Dispatch& bind(unsigned id, etna::Binding binding);

    Dispatch& groups(uint32_t x, uint32_t y = 1, uint32_t z = 1);

    template <class T>
    Dispatch& push(const T& constant)
    {
      pushConstant = std::unique_ptr<GenericPushConstant>(new PushConstant<T>(constant));
      return *this;
    }

  private:
    vk::CommandBuffer* cmdBuf = nullptr;
    etna::ComputePipeline* pipeline = nullptr;
    etna::PersistentDescriptorSet* descriptorSet = nullptr;

    uint32_t groupCountX = 1, groupCountY = 1, groupCountZ = 1;

    std::vector<etna::Binding> bindings{};
    std::unique_ptr<GenericPushConstant> pushConstant{};

    std::string programName;
  };

  void init();

  Dispatch dispatch(vk::CommandBuffer& command_buffer)
  {
    assert(m_inited);
    Dispatch dispatch(command_buffer, m_etnaPipeline);
    dispatch.programName = m_programName;
    if (m_descriptorSet.isValid()) dispatch.descriptorSet = &m_descriptorSet;
    return dispatch;
  }

  ComputeShader& shaderPath(const std::filesystem::path& value)
  {
    assert(!m_inited);
    m_shaderPath = value;
    return *this;
  }

  ComputeShader& programName(const std::string& value)
  {
    assert(!m_inited);
    m_programName = value;
    return *this;
  }

  ComputeShader& addPersistentBinding(Texture& texture, etna::Sampler& sampler)
  {
    assert(!m_inited);
    unsigned arrayElem = static_cast<unsigned>(m_bindlessBindings.size());
    m_bindlessBindings.emplace_back(texture.getBinding(0, sampler, arrayElem));
    return *this;
  }

private:
  bool m_inited = false;

  std::filesystem::path m_shaderPath = "";
  std::string m_programName = "";
  etna::ComputePipeline::CreateInfo m_creationInfo;

  etna::ComputePipeline m_etnaPipeline;

  etna::PersistentDescriptorSet m_descriptorSet;
  std::vector<etna::Binding> m_bindlessBindings;
};

}


