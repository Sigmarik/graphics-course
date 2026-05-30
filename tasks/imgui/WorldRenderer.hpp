#pragma once

#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <etna/Buffer.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <glm/glm.hpp>

#include "scene/SceneManager.hpp"
#include "wsi/Keyboard.hpp"
#include "FrameMachine.hpp"

#include "FramePacket.hpp"
#include "etna/ComputePipeline.hpp"


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
  void initDecals();
  void initGBuffers();
  void initClusters();

  void assignLightClusters(vk::CommandBuffer cmd_buf);
  void renderToGBuffers(vk::CommandBuffer cmd_buf);
  void applyDecals(vk::CommandBuffer cmd_buf);
  void applyLighting(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

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
  unsigned currentPointLightCount;
  unsigned pointLightCount;

  struct GBuffers
  {
    etna::Image albedo;
    etna::Image normal;
    etna::Image depth;

    template <class F>
    void forEachBuffer(F&& functor)
    {
      functor(albedo);
      functor(normal);
      functor(depth);
    }
  };

  FrameMachine<GBuffers, 2> geomBuffers;

  struct PushConstants
  {
    glm::mat4x4 projView;
    glm::mat4x4 model;
  } pushConst2M;

  struct DeferredPushConstants
  {
    glm::mat4x4 invProj;
    glm::mat4x4 view;
    unsigned numberOfElements;
  } deferredPushConst2M;

  struct Decal
  {
    glm::vec3 position = glm::vec3(0.0, 0.0, 0.0);
    float _padding0 = 0.0f;
    glm::vec3 direction = glm::vec3(0.0, -1.0, 0.0);
    float size = 1.0;
    float orientation = 0.0;
    float depth = 1.0;
    float _padding1[2];
  };
  etna::Buffer decals;
  unsigned currentDecalCount;
  unsigned decalCount;

  static constexpr unsigned DEFERRED_CLUSTER_COUNT_LATERAL = 30;
  static constexpr unsigned DEFERRED_CLUSTER_COUNT_VERTICAL = 50;

  struct DeferredCluster
  {
    uint32_t count;
    static constexpr unsigned ELEMENTS_PER_CLUSTER = 100;
    uint32_t elementIndices[ELEMENTS_PER_CLUSTER] = {};
  };
  etna::Buffer deferredClusters;

  glm::mat4x4 worldViewProj;
  glm::mat4x4 worldView;
  glm::mat4x4 worldInvProj;
  glm::mat4x4 lightMatrix;

  etna::GraphicsPipeline staticMeshPipeline{};
  etna::GraphicsPipeline deferredLightingPipeline{};
  etna::GraphicsPipeline decalPipeline{};

  etna::ComputePipeline lightClusterAssignmentPipeline{};

  glm::uvec2 resolution;

  etna::Sampler sampler;
};
