#pragma once

#include <glm/glm.hpp>

#include "wsi/OsWindowingManager.hpp"
#include "scene/Camera.hpp"

#include "Renderer.hpp"
#include "etna/RenderTargetStates.hpp"

namespace spg
{

class RenderPrimitive;
class RenderObject;

class App
{
public:
  App();
  virtual ~App() = default;

  void run();

  friend class Renderer;
  friend class WorldRenderer;

  glm::mat4 getWorldView() const { return worldView; }
  glm::mat4 getWorldInvProj() const { return worldInvProj; }
  glm::mat4 getWorldViewProj() const { return worldViewProj; }

  vk::CommandBuffer& getCmdBuf() const { return *currentCmdBuf; }

  vk::Image& getCurrentTargetImage() const { return *currentTargetImage; }
  vk::ImageView& getCurrentTargetImageView() const { return *currentTargetImageView; }

  etna::RenderTargetState::AttachmentParams getScreenAttachment() const
  {
    return { .image = *currentTargetImage, .view = *currentTargetImageView };
  };

  glm::uvec2 getResolution() const { return mainWindow->getResolution(); }

  etna::Sampler& getDefaultSampler() const { return renderer->getSampler(); }

protected:
  virtual void renderGui() {}
  virtual void render() = 0;
  virtual void initialize() = 0;

private:
  void processInput(float dt);
  void drawFrame();

  void moveCam(Camera& cam, const Keyboard& kb, float dt);
  void rotateCam(Camera& cam, const Mouse& ms, float dt);

private:
  OsWindowingManager windowing;
  std::unique_ptr<OsWindow> mainWindow;

  float camMoveSpeed = 1;
  float camRotateSpeed = 0.1f;
  float zoomSensitivity = 2.0f;
  Camera mainCam;

  std::unique_ptr<Renderer> renderer;

  glm::mat4 worldView;
  glm::mat4 worldInvProj;
  glm::mat4 worldViewProj;

  vk::CommandBuffer* currentCmdBuf;
  vk::Image* currentTargetImage;
  vk::ImageView* currentTargetImageView;
};
}
