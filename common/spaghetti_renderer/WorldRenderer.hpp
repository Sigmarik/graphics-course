#pragma once

#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <glm/glm.hpp>

#include "wsi/Keyboard.hpp"

#include "FramePacket.hpp"

namespace spg
{
class App;

class WorldRenderer
{
public:
  WorldRenderer();

  void allocateOwnResources(glm::uvec2 swapchain_resolution);

  void debugInput(const Keyboard& kb);
  void drawGui(App& app);
  void renderWorld(App& app,
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

  etna::Sampler& getSampler() { return sampler; }
  const etna::Sampler& getSampler() const { return sampler; }

private:
  etna::Image mainViewDepth;
  glm::uvec2 resolution;
  etna::Sampler sampler;
};
}
