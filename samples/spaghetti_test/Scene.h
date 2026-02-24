#pragma once

#include <spaghetti_renderer/spaghetti.h>

class Scene : public spg::App
{
public:
  Scene() = default;

protected:
  void render() override;
};
