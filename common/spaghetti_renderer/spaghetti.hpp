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

  float getAspect() const
  {
    return static_cast<float>(mainWindow->getResolution().x) / static_cast<float>(mainWindow->getResolution().y);
  }

  glm::mat4 getWorldView() const { return getCam().viewTm(); }
  glm::mat4 getWorldInvProj() const
  {
    const float aspect = getAspect();
    return glm::inverse(getCam().projTm(aspect));
  }
  glm::mat4 getWorldViewProj() const
  {
    const float aspect = getAspect();
    return getCam().projTm(aspect) * getCam().viewTm();
  }

  vk::CommandBuffer& getCmdBuf() const { return *currentCmdBuf; }

  vk::Image& getCurrentTargetImage() const { return *currentTargetImage; }
  vk::ImageView& getCurrentTargetImageView() const { return *currentTargetImageView; }

  etna::RenderTargetState::AttachmentParams getScreenAttachment() const
  {
    return { .image = *currentTargetImage, .view = *currentTargetImageView };
  };

  glm::uvec2 getResolution() const { return mainWindow->getResolution(); }

  etna::Sampler& getDefaultSampler() const { return renderer->getSampler(); }

  float getDeltaTime() const { return lastDeltaTime; }

  void overrideCamera(Camera& camera) { camOverride = &camera; }
  void clearCameraOverride() { camOverride = nullptr; }

protected:
  virtual void renderGui() {}
  virtual void render() = 0;
  virtual void initialize() = 0;

  Camera& getCam() { return camOverride != nullptr ? *camOverride : mainCam; }
  const Camera& getCam() const { return camOverride != nullptr ? *camOverride : mainCam; }

  Camera mainCam;
  Camera* camOverride = nullptr;

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
  float lastDeltaTime = 0.0f;

  std::unique_ptr<Renderer> renderer;

  vk::CommandBuffer* currentCmdBuf;
  vk::Image* currentTargetImage;
  vk::ImageView* currentTargetImageView;
};
}
