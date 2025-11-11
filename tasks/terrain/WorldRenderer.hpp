#pragma once

#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <etna/Buffer.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <glm/glm.hpp>

#include "scene/SceneManager.hpp"
#include "wsi/Keyboard.hpp"

#include "FramePacket.hpp"

#include <set>


class WorldRenderer
{
public:
  WorldRenderer();

  void loadShaders();
  void allocateResources(glm::uvec2 swapchain_resolution);
  void setupPipelines(vk::Format swapchain_format);

  void debugInput(const Keyboard& kb);
  void update(const FramePacket& packet);
  void drawGui();
  void renderWorld(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

  struct Chunk
  {
    glm::vec2 position{};
    float size = 16.0;
    uint32_t offset = 0;
  };
  static constexpr unsigned CHUNK_RESOLUTION = 17;

private:
  etna::Image mainViewDepth;

  struct ChunkVertex
  {
    glm::ivec2 position{};
  };

  struct TerrainVertex
  {
    float elevation = 0.0;
    float _padding = 0;
    glm::vec2 normal{};
  };

  TerrainVertex terrainAtPosition(const glm::vec2& pos) const;

  etna::Buffer chunkElevationBuffer{};
  etna::Buffer chunkMap{};

  etna::Buffer chunkMeshVertices{};
  etna::Buffer chunkMeshIndices{};

  void generateChunkMesh();

  struct BindingManager
  {
    std::map<uint64_t, uint32_t> bindings{};
    std::set<uint32_t> freeBindings{};

    uint32_t getBinding(glm::ivec2 position);
    void freeBinding(glm::vec2 position);
  };

  void blitChunk(Chunk& chunk);
  std::vector<Chunk> generateChunkMap(const glm::vec2& camera_pos) const;

  uint32_t mapChunks(const glm::vec2& camera_pos);

  struct PushConstants
  {
    glm::mat4x4 projView;
    glm::mat4x4 model;
  } pushConst2M{};

  glm::mat4x4 worldViewProj{};
  glm::mat4x4 lightMatrix{};

  glm::vec3 cameraPos{};

  etna::GraphicsPipeline terrainPipeline{};

  glm::uvec2 resolution{};
};
