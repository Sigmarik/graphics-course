#pragma once

#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <etna/Buffer.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <glm/glm.hpp>

#include "scene/SceneManager.hpp"
#include "wsi/Keyboard.hpp"

#include "FramePacket.hpp"


class WorldRenderer
{
public:
  WorldRenderer();

  void loadScene(std::filesystem::path path);

  void loadShaders();
  void allocateResources(glm::uvec2 swapchain_resolution);
  void setupPipelines(vk::Format swapchain_format);

  void debugInput(const Keyboard& kb);
  void update(const FramePacket& packet);
  void drawGui();
  void renderWorld(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

private:
  void renderScene(
    vk::CommandBuffer cmd_buf, const glm::mat4x4& glob_tm, vk::PipelineLayout pipeline_layout);

  void initLights();
  void initGBuffers();

  void renderToGBuffers(vk::CommandBuffer cmd_buf);
  void applyLighting(vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

  struct PointLight
  {
    glm::vec3 position;
    float _padding = 0;
    glm::vec3 color;
    float radius;
  };

  std::vector<PointLight> genPointLights();

private:
  std::unique_ptr<SceneManager> sceneMgr;

  etna::Image mainViewDepth;
  etna::Buffer constants;

  etna::Buffer pointLights;

  struct GBuffers
  {
    etna::Image albedo;
    etna::Image normal;
    etna::Image depth;
  };

  GBuffers geomBuffers;

  struct PushConstants
  {
    glm::mat4x4 projView;
    glm::mat4x4 model;
  } pushConst2M;

  struct DeferredPushConstants
  {
    glm::mat4x4 invProj;
    glm::mat4x4 view;
    unsigned numberOfLights;
  } deferredPushConst2M;

  glm::mat4x4 worldViewProj;
  glm::mat4x4 worldView;
  glm::mat4x4 worldInvProj;
  glm::mat4x4 lightMatrix;

  etna::GraphicsPipeline staticMeshPipeline{};
  etna::GraphicsPipeline deferredLightingPipeline{};

  glm::uvec2 resolution;

  etna::Sampler sampler;
};
