#pragma once

#include "etna/BlockingTransferHelper.hpp"
#include "etna/ComputePipeline.hpp"
#include "etna/GraphicsPipeline.hpp"


#include <chrono>

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "wsi/OsWindowingManager.hpp"


class App
{
public:
  App();
  ~App();

  void run();

private:
  void drawFrame();

  void updateParams();

  void importTextures();

  void approximateBrightnessDistribution(vk::CommandBuffer& buffer);

private:
  OsWindowingManager windowing;
  std::unique_ptr<OsWindow> osWindow;

  glm::uvec2 resolution;
  bool useVsync;

  std::unique_ptr<etna::Window> vkWindow;
  std::unique_ptr<etna::PerFrameCmdMgr> commandManager;

  struct ShaderParams
  {
    uint32_t resolutionX, resolutionY;
    float mouseX, mouseY;
    float time;
  };

  struct TonemappingParams
  {
    uint32_t resolutionX, resolutionY;
    float avg, var;
  };

  struct AveragingParams
  {
    uint32_t resolutionX, resolutionY;
    uint32_t shiftX, shiftY;
  };

  etna::Sampler sampler;

  etna::Image skyTexture;
  etna::Image ballTexture;
  static constexpr glm::uvec2 BALL_TEXTURE_RESOLUTION{2048, 2048};

  etna::Image hdrFrameCopy;

  etna::Image hdrFrame;
  etna::Image hdrFrameBackBuf;

  std::unique_ptr<etna::BlockingTransferHelper> transferHelper;
  etna::Buffer brightnessReadbackBuffer;

  etna::GraphicsPipeline intermediatePipeline;
  etna::GraphicsPipeline mainPipeline;
  etna::GraphicsPipeline toneMappingPipeline;
  etna::ComputePipeline avgBrightnessPipeline;

  ShaderParams params;
  TonemappingParams tonemappingParams;
  AveragingParams averagingParams;

  std::chrono::steady_clock::time_point startTime;
};
