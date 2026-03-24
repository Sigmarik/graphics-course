#include "DeferredTextureBunch.h"

void DeferredTextureBunch::init(vk::CommandBuffer* cmdBuf)
{
  depth
    .size(res.x, res.y)
    .useDepthStencil()
    .format(vk::Format::eD32Sfloat)
    .name("depth")
    .init(cmdBuf);

  albedo
    .size(res.x, res.y)
    .useColorAttachment()
    .useSampled()
    .format(vk::Format::eR8G8B8A8Unorm)
    .name("albedo")
    .init(cmdBuf);

  normalEmissive
    .size(res.x, res.y)
    .useColorAttachment()
    .useSampled()
    .format(vk::Format::eR16G16B16A16Sfloat)
    .name("normalEmissive")
    .init(cmdBuf);
}