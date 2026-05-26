#pragma once

#include "spaghetti_renderer/spaghetti.hpp"
#include "spaghetti_renderer/primitives/Buffer.hpp"
#include "spaghetti_renderer/primitives/ComputeShader.hpp"
#include "spaghetti_renderer/primitives/Texture.hpp"
#include "spaghetti_renderer/primitives/VertexFragmentShader.hpp"

struct Particle
{
  glm::vec3 position{};
  glm::vec3 velocity{};
  float ttl = 0.0f;
  float size = 0.1f;
  glm::u8vec3 color{255, 0, 0};
  uint8_t transparency = 128;
};

class Emitter
{
public:
  float positionVariation = 0.0f;
  float velocityVariation = 0.0f;
  float sizeVariation = 0.0f;
  float transparencyVariation = 0.0f;
  float spawnDt = 0.1f;
  float timeBeforeNextSpawn = 0.0f;
  float ttlVariation = 0.0f;
  Particle particleTemplate;
};

class Particles
{
public:
  void init(spg::App& app, const std::vector<Emitter>& emitters,
    vk::Format colorFormat, vk::Format depthFormat);

  void tick(spg::App& app, float deltaTime);
  void draw(spg::App& app, spg::Texture& color, spg::Texture& depth);

private:
  spg::VertexFragmentShader renderShader{};

  spg::ComputeShader particleIterator{};
  spg::ComputeShader emitterIterator{};
  spg::ComputeShader depthSorter{};

  spg::Buffer gpuParticles{};
  spg::Buffer gpuEmitters{};
  spg::Buffer sortedIndices{};
  spg::Buffer freeParticles{};
  spg::Buffer freeParticleCount{};

  uint16_t maxParticles = 1024;
};
