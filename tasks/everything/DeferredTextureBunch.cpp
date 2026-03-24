#include "DeferredTextureBunch.h"

void DeferredTextureBunch::init(vk::CommandBuffer* cmdBuf)
{
  depth
    .size(res.x, res.y)
    .useDepthStencil()
    .useSampled()
    .format(DEPTH_FORMAT)
    .name("depth")
    .init(cmdBuf);

  albedo
    .size(res.x, res.y)
    .useColorAttachment()
    .useSampled()
    .format(ALBEDO_FORMAT)
    .name("albedo")
    .init(cmdBuf);

  normalEmissive
    .size(res.x, res.y)
    .useColorAttachment()
    .useSampled()
    .format(NORMAL_EMISSIVE_FORMAT)
    .name("normalEmissive")
    .init(cmdBuf);
}