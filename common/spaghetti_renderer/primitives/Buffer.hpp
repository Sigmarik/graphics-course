#pragma once
#include "vk_mem_alloc.h"
#include "etna/Buffer.hpp"
#include "etna/DescriptorSet.hpp"

namespace spg
{
class Buffer
{
public:
  Buffer() = default;
  Buffer(etna::Buffer buffer);

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&&) = default;

  Buffer& size(size_t value) { assert(!m_inited); m_size = value; return *this; }
  Buffer& useIndirect() { assert(!m_inited); m_usage |= vk::BufferUsageFlagBits::eIndirectBuffer; return *this; }
  Buffer& useStorage() { assert(!m_inited); m_usage |= vk::BufferUsageFlagBits::eStorageBuffer; return *this; }
  Buffer& useIndex() { assert(!m_inited); m_usage |= vk::BufferUsageFlagBits::eIndexBuffer; return *this; }
  Buffer& useVertex() { assert(!m_inited); m_usage |= vk::BufferUsageFlagBits::eVertexBuffer; return *this; }
  Buffer& memCpu2Gpu() { assert(!m_inited); m_memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU; return *this; }
  Buffer& memGpuOnly() { assert(!m_inited); m_memUsage = VMA_MEMORY_USAGE_GPU_ONLY; return *this; }
  Buffer& memGpu2Cpu() { assert(!m_inited); m_memUsage = VMA_MEMORY_USAGE_GPU_TO_CPU; return *this; }
  Buffer& name(std::string value) { assert(!m_inited); m_name = value; return *this; }

  void init();

  etna::Buffer& raw() { return m_etnaBuffer; }

  void prepareForShaderRead(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage);
  void prepareForShaderWrite(vk::CommandBuffer& cmd_buf, vk::PipelineStageFlagBits2 stage = vk::PipelineStageFlagBits2::eComputeShader);

  void* data() { return raw().data(); }

  template<class T>
  T* dataAs() { return static_cast<T*>(data()); }

  etna::Binding getBinding(unsigned id) { return etna::Binding{id, m_etnaBuffer.genBinding()}; }

  template <class T>
  void copyFrom(const T& source, size_t count)
  {
    assert(sizeof(T) * count <= m_size);
    std::memcpy(data(), &source, sizeof(T) * count);
  }

  template <class T>
  void copyFrom(const T& source)
  {
    size_t count = m_size / sizeof(T);
    assert(sizeof(T) * count == m_size);
    copyFrom(source, count);
  }

private:
  bool m_inited = false;

  size_t m_size = 0;
  vk::BufferUsageFlags m_usage = vk::BufferUsageFlagBits::eStorageBuffer;
  VmaMemoryUsage m_memUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  std::string m_name = "";

  etna::Buffer m_etnaBuffer;
};
}
