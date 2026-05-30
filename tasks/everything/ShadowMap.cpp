#include "ShadowMap.h"


void ShadowMap::init(spg::App& app, const glm::vec3& light_direction)
{
  for (spg::Texture& map : m_textures)
  {
    map.name("shadowMap")
      .size(glm::uvec2(1024, 1024))
      .format(vk::Format::eD32Sfloat)
      .useDepthStencil()
      .useSampled()
      .init(&app.getCmdBuf());
  }

  float smallestCamSize = 5;
  for (unsigned camIdx = 0; camIdx < m_cameras.size(); camIdx++)
  {
    Camera& cam = m_cameras[camIdx];
    cam.orthoHeight = smallestCamSize;
    smallestCamSize *= 3;
    cam.lookAt(glm::vec3(0),  glm::normalize(light_direction), glm::vec3(0, 1, 0));
    cam.orthographic = true;
  }
}

void ShadowMap::updateCameraPositions(const glm::vec3& main_camera_pos)
{
  glm::uvec2 res = glm::uvec2(MAP_RESOLUTION, MAP_RESOLUTION);
  for (Camera& camera : m_cameras)
  {
    camera.setPixelAccuratePosition(main_camera_pos - camera.forward() * 500.0f, res);
  }
}