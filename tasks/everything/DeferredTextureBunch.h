#pragma once

#include <spaghetti_renderer/primitives/Texture.hpp>
#include <glm/vec2.hpp>

struct DeferredTextureBunch
{
  spg::Texture depth;
  spg::Texture albedo;
  spg::Texture normalEmissive;
  glm::uvec2 res;

  static constexpr vk::Format DEPTH_FORMAT = vk::Format::eD32Sfloat;
  static constexpr vk::Format ALBEDO_FORMAT = vk::Format::eR8G8B8A8Unorm;
  static constexpr vk::Format NORMAL_EMISSIVE_FORMAT = vk::Format::eR8G8B8A8Snorm;

  DeferredTextureBunch& resolution(glm::uvec2 resol) { res = resol; return *this; }

  void init(vk::CommandBuffer* cmdBuf);
};
