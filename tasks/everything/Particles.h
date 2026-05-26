#pragma once

#include "spaghetti_renderer/spaghetti.hpp"
#include "spaghetti_renderer/primitives/Buffer.hpp"
#include "spaghetti_renderer/primitives/ComputeShader.hpp"
#include "spaghetti_renderer/primitives/Texture.hpp"
#include "spaghetti_renderer/primitives/VertexFragmentShader.hpp"

#include <cstdint>

struct Particle
{
  float px = 0.0f;
  float py = 0.0f;
  float pz = 0.0f;
  float vx = 0.0f;
  float vy = 0.0f;
  float vz = 0.0f;
  float ttl = 0.0f;
  float size = 0.1f;
  uint32_t colorRGBA = 0xFF0000FFu;
};

inline uint32_t packColorRGBA(const glm::u8vec3& color, uint8_t alpha)
{
  return static_cast<uint32_t>(color.r)
    | (static_cast<uint32_t>(color.g) << 8u)
    | (static_cast<uint32_t>(color.b) << 16u)
    | (static_cast<uint32_t>(alpha) << 24u);
}

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
