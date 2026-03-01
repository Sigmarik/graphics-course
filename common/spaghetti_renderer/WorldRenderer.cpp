#include "WorldRenderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>

#include <imgui.h>

#include "spaghetti.hpp"

namespace spg
{
WorldRenderer::WorldRenderer()
{
}

void WorldRenderer::allocateOwnResources(glm::uvec2 swapchain_resolution)
{
  resolution = swapchain_resolution;

  auto& ctx = etna::get_context();

  mainViewDepth = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "main_view_depth",
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
  });

  sampler = etna::Sampler(etna::Sampler::CreateInfo{
    .filter = vk::Filter::eLinear,
    .addressMode = vk::SamplerAddressMode::eRepeat,
    .name = "default_sampler"});
}

void WorldRenderer::debugInput(const Keyboard&) {}

void WorldRenderer::drawGui(App& app)
{
  ImGui::Begin("Simple render settings");

  app.renderGui();

  ImGui::Text(
    "Application average %.3f ms/frame (%.1f FPS)",
    1000.0f / ImGui::GetIO().Framerate,
    ImGui::GetIO().Framerate);
  ImGui::End();
}

void WorldRenderer::renderWorld(App& app,
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderWorld);

  app.currentCmdBuf = &cmd_buf;
  app.currentTargetImage = &target_image;
  app.currentTargetImageView = &target_image_view;

  app.render();
}
}
