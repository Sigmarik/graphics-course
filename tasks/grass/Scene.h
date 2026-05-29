#pragma once

#include <spaghetti_renderer/primitives/VertexFragmentShader.hpp>
#include <spaghetti_renderer/spaghetti.hpp>

class Scene : public spg::App
{
protected:
  void initialize() override;
  void render() override;
  void renderGui() override;

private:
  spg::Buffer vertices{};
  spg::Buffer indices{};

  unsigned numberOfGrassBlades = 5000000;

  spg::Texture depth{};
  spg::VertexFragmentShader shader{};
};
