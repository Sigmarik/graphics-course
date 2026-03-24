#pragma once

#include <spaghetti_renderer/primitives/Texture.hpp>
#include <glm/vec2.hpp>

struct DeferredTextureBunch
{
  spg::Texture depth;
  spg::Texture albedo;
  spg::Texture normalEmissive;
  glm::uvec2 res;

  DeferredTextureBunch& resolution(glm::uvec2 resol) { res = resol; return *this; }

  void init(vk::CommandBuffer* cmdBuf);
};
