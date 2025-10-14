#pragma once

#include "etna/BlockingTransferHelper.hpp"
#include "etna/GraphicsPipeline.hpp"


#include <chrono>

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
#include <etna/ComputePipeline.hpp>
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

  etna::Sampler sampler;

  etna::Image skyTexture;
  etna::Image ballTexture;
  static constexpr glm::uvec2 BALL_TEXTURE_RESOLUTION{2048, 2048};

  etna::GraphicsPipeline intermediatePipeline;
  etna::GraphicsPipeline mainPipeline;

  ShaderParams params;

  std::chrono::steady_clock::time_point startTime;
};
