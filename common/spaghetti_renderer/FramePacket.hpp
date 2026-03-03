#pragma once

#include <scene/Camera.hpp>

namespace spg
{
struct FramePacket
{
  Camera mainCam;
  float currentTime = 0;
};
}
