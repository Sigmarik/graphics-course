#pragma once

#include "spaghetti_renderer/primitives/FragmentOnlyShader.hpp"


#include <spaghetti_renderer/spaghetti.hpp>

class Scene : public spg::App
{
protected:
  void initialize() override;
  void render() override;

private:
  spg::FragmentOnlyShader fullRed;
  spg::Buffer colorBuffer;
};
