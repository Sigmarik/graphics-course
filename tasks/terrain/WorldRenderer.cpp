#include "WorldRenderer.hpp"

#include "BoundingBox.hpp"
#include "noise.h"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>


WorldRenderer::WorldRenderer() {}

void WorldRenderer::allocateResources(glm::uvec2 swapchain_resolution)
{
  resolution = swapchain_resolution;

  auto& ctx = etna::get_context();

  mainViewDepth = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "main_view_depth",
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
  });

  auto chunks = generateChunkMap(glm::vec2(0.0));

  chunkElevationBuffer = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = chunks.size() * sizeof(TerrainVertex) * CHUNK_RESOLUTION * CHUNK_RESOLUTION,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "elevation",
  });
  chunkElevationBuffer.map();

  chunkMap = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = chunks.size() * sizeof(Chunk),
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "chunkMap",
  });
  chunkMap.map();

  chunkMeshIndices = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = sizeof(uint32_t) * (CHUNK_RESOLUTION - 1) * (CHUNK_RESOLUTION - 1) * 2 * 3,
    .bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "terrainChunkIndices",
  });
  chunkMeshIndices.map();

  chunkMeshVertices = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = sizeof(ChunkVertex) * CHUNK_RESOLUTION * CHUNK_RESOLUTION,
    .bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "terrainChunkVertices",
  });
  chunkMeshVertices.map();

  generateChunkMesh();
}

void WorldRenderer::loadShaders()
{
  etna::create_program(
    "static_mesh_material",
    {TERRAIN_SHADERS_ROOT "static_mesh.frag.spv", TERRAIN_SHADERS_ROOT "static_mesh.vert.spv"});
  etna::create_program("static_mesh", {TERRAIN_SHADERS_ROOT "static_mesh.vert.spv"});
}

void WorldRenderer::setupPipelines(vk::Format swapchain_format)
{
  auto format = etna::VertexByteStreamFormatDescription{
    .stride = sizeof(ChunkVertex),
    .attributes = {
      etna::VertexByteStreamFormatDescription::Attribute{
        .format = vk::Format::eR32G32Sint,
        .offset = 0,
      },
    }};
  etna::VertexShaderInputDescription sceneVertexInputDesc{
    .bindings = {etna::VertexShaderInputDescription::Binding{
      .byteStreamDescription = format,
    }},
  };

  auto& pipelineManager = etna::get_context().getPipelineManager();

  terrainPipeline = {};
  terrainPipeline = pipelineManager.createGraphicsPipeline(
    "static_mesh_material",
    etna::GraphicsPipeline::CreateInfo{
      .vertexShaderInput = sceneVertexInputDesc,
      .rasterizationConfig =
        vk::PipelineRasterizationStateCreateInfo{
          .polygonMode = vk::PolygonMode::eFill,
          .cullMode = vk::CullModeFlagBits::eBack,
          .frontFace = vk::FrontFace::eCounterClockwise,
          .lineWidth = 1.f,
        },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {swapchain_format},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });
}

void WorldRenderer::debugInput(const Keyboard&) {}

void WorldRenderer::update(const FramePacket& packet)
{
  ZoneScoped;

  // calc camera matrix
  {
    const float aspect = float(resolution.x) / float(resolution.y);
    worldViewProj = packet.mainCam.projTm(aspect) * packet.mainCam.viewTm();
    cameraPos = packet.mainCam.position;
  }
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderWorld);

  {
    ETNA_PROFILE_GPU(cmd_buf, renderForward);

    etna::RenderTargetState renderTargets(
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      {{.image = target_image, .view = target_image_view}},
      {.image = mainViewDepth.get(), .view = mainViewDepth.getView({})});

    uint32_t chunkCount = mapChunks(glm::vec2(cameraPos.z, cameraPos.x));

    auto intermediateInfo = etna::get_shader_program("static_mesh_material");
    auto set = etna::create_descriptor_set(
      intermediateInfo.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, chunkElevationBuffer.genBinding()},
        etna::Binding{1, chunkMap.genBinding()},
      });

    vk::DescriptorSet vkSet = set.getVkSet();

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, terrainPipeline.getVkPipeline());

    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      terrainPipeline.getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);

    cmd_buf.bindVertexBuffers(0, {chunkMeshVertices.get()}, {0});
    cmd_buf.bindIndexBuffer(chunkMeshIndices.get(), 0, vk::IndexType::eUint32);

    pushConst2M.projView = worldViewProj;

    cmd_buf.pushConstants<PushConstants>(
      terrainPipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, {pushConst2M});

    cmd_buf.drawIndexed(
      (CHUNK_RESOLUTION - 1) * (CHUNK_RESOLUTION - 1) * 2 * 3, chunkCount, 0, 0, 0);
  }

  etna::flush_barriers(cmd_buf);
}

static float elevationAt(const glm::vec2& pos)
{
  static const siv::BasicPerlinNoise<float> PERLIN_NOISE(0);

  glm::vec2 scaledPosition = pos / 100.0f;
  float noise = PERLIN_NOISE.octave2D_01(scaledPosition.x, scaledPosition.y, 5);
  return noise * noise * 60.0f * (glm::length(pos) * 0.001f + 1.0f);
}

WorldRenderer::TerrainVertex WorldRenderer::terrainAtPosition(const glm::vec2& pos) const
{
  // TODO: Implement something more interesting
  TerrainVertex vtx;
  vtx.elevation = elevationAt(pos);
  constexpr float EPS = 0.01f;
  glm::vec3 dx = glm::vec3(EPS, elevationAt(pos + glm::vec2(EPS, 0.0)) - vtx.elevation, 0.0);
  glm::vec3 dy = glm::vec3(0.0, elevationAt(pos + glm::vec2(0.0, EPS)) - vtx.elevation, EPS);
  glm::vec3 normal = glm::normalize(glm::cross(dy, dx));
  vtx.normal = glm::vec2(normal.z, normal.x);
  return vtx;
}

void WorldRenderer::generateChunkMesh()
{
  std::vector<ChunkVertex> vertices;
  std::vector<uint32_t> indices;

  for (unsigned dx = 0; dx < CHUNK_RESOLUTION; dx++)
  {
    for (unsigned dy = 0; dy < CHUNK_RESOLUTION; dy++)
    {
      ChunkVertex vtx;
      vtx.position = glm::ivec2(dx, dy);
      vertices.emplace_back(vtx);
    }
  }

  for (unsigned dx = 0; dx + 1 < CHUNK_RESOLUTION; dx++)
  {
    for (unsigned dy = 0; dy + 1 < CHUNK_RESOLUTION; dy++)
    {
      uint32_t base = dx * CHUNK_RESOLUTION + dy;

      //  *--*
      //  | /
      //  |/
      //  *
      indices.emplace_back(base);
      indices.emplace_back(base + CHUNK_RESOLUTION);
      indices.emplace_back(base + 1);

      //     *
      //    /|
      //   / |
      //  *--*
      indices.emplace_back(base + 1);
      indices.emplace_back(base + CHUNK_RESOLUTION);
      indices.emplace_back(base + CHUNK_RESOLUTION + 1);
    }
  }

  std::memcpy(chunkMeshVertices.data(), vertices.data(), sizeof(ChunkVertex) * vertices.size());
  std::memcpy(chunkMeshIndices.data(), indices.data(), sizeof(uint32_t) * indices.size());
}

void WorldRenderer::BindingManager::genBindings(WorldRenderer& world, std::vector<Chunk>& chunks)
{
  if (bindings.empty())
  {
    unsigned id = 0;
    for (Chunk& chunk : chunks)
    {
      chunk.offset = id * CHUNK_RESOLUTION * CHUNK_RESOLUTION;
      world.blitChunk(chunk);
      ++id;
      bindings.emplace(ChunkId::from_chunk(chunk), chunk.offset);
    }
    return;
  }

  std::vector<bool> allocated(chunks.size(), false);
  std::vector<Chunk*> chunkQueue;

  std::map<ChunkId, uint32_t> newAllocations;

  for (Chunk& chunk : chunks)
  {
    auto found = bindings.find(ChunkId::from_chunk(chunk));
    if (found == bindings.end())
    {
      chunkQueue.emplace_back(&chunk);
    }
    else
    {
      newAllocations.emplace(found->first, found->second);
      chunk.offset = found->second;
      allocated[chunk.offset / CHUNK_RESOLUTION / CHUNK_RESOLUTION] = true;
    }
  }

  std::vector<uint32_t> unusedBindings;
  for (uint32_t bnd = 0; bnd < allocated.size(); bnd++)
  {
    if (!allocated[bnd])
      unusedBindings.emplace_back(bnd);
  }

  for (Chunk* chunk : chunkQueue)
  {
    chunk->offset = unusedBindings.back() * CHUNK_RESOLUTION * CHUNK_RESOLUTION;
    unusedBindings.pop_back();
    world.blitChunk(*chunk);
    newAllocations.emplace(ChunkId::from_chunk(*chunk), chunk->offset);
  }

  bindings = std::move(newAllocations);
}

void WorldRenderer::blitChunk(Chunk& chunk)
{
  std::vector<TerrainVertex> vertices;
  // vertices.reserve(CHUNK_RESOLUTION * CHUNK_RESOLUTION);
  for (unsigned dx = 0; dx < CHUNK_RESOLUTION; ++dx)
  {
    for (unsigned dy = 0; dy < CHUNK_RESOLUTION; ++dy)
    {
      glm::vec2 pos = chunk.position;
      pos.x += static_cast<float>(dx) / (CHUNK_RESOLUTION - 1) * chunk.size;
      pos.y += static_cast<float>(dy) / (CHUNK_RESOLUTION - 1) * chunk.size;
      vertices.emplace_back(terrainAtPosition(pos));
    }
  }
  std::memcpy(
    chunkElevationBuffer.data() + chunk.offset * sizeof(TerrainVertex),
    vertices.data(),
    sizeof(TerrainVertex) * vertices.size());
}

struct ChunkArray
{
  std::vector<WorldRenderer::Chunk> chunks;

  void add(glm::ivec2 pos, unsigned size)
  {
    WorldRenderer::Chunk chunk;
    chunk.position = pos;
    chunk.size = size;
    chunk.offset = static_cast<uint32_t>(
      chunks.size() * WorldRenderer::CHUNK_RESOLUTION * WorldRenderer::CHUNK_RESOLUTION);
    chunks.emplace_back(std::move(chunk));
  }
};

static constexpr unsigned RING_WIDTH = 5;

static void cellify(
  ChunkArray& chunks,
  const glm::ivec2& start,
  const glm::vec2& center,
  unsigned size,
  unsigned count,
  unsigned subdivisions)
{
  if (subdivisions == 0)
  {
    for (unsigned idX = 0; idX < count; ++idX)
    {
      for (unsigned idY = 0; idY < count; ++idY)
      {
        chunks.add(start + glm::ivec2(idX, idY) * static_cast<int>(size), size);
      }
    }
    return;
  }

  assert(count > RING_WIDTH * 2);

  glm::vec2 approxCenter = glm::vec2(start) + static_cast<float>(size * count) / 2.0f;
  glm::ivec2 compensation = glm::floor((center - approxCenter) / static_cast<float>(size));

  assert(abs(compensation.x) <= RING_WIDTH);
  assert(abs(compensation.y) <= RING_WIDTH);

  for (unsigned idX = 0; idX < count; ++idX)
  {
    for (unsigned idY = 0; idY < RING_WIDTH + compensation.y; ++idY)
    {
      chunks.add(start + glm::ivec2(idX, idY) * static_cast<int>(size), size);
    }
  }

  for (unsigned idX = 0; idX < count; ++idX)
  {
    for (unsigned idY = count - RING_WIDTH + compensation.y; idY < count; ++idY)
    {
      chunks.add(start + glm::ivec2(idX, idY) * static_cast<int>(size), size);
    }
  }

  for (unsigned idX = 0; idX < RING_WIDTH + compensation.x; ++idX)
  {
    for (unsigned idY = RING_WIDTH + compensation.y; idY + RING_WIDTH - compensation.y < count;
         ++idY)
    {
      chunks.add(start + glm::ivec2(idX, idY) * static_cast<int>(size), size);
    }
  }

  for (unsigned idX = count - RING_WIDTH + compensation.x; idX < count; ++idX)
  {
    for (unsigned idY = RING_WIDTH + compensation.y; idY + RING_WIDTH - compensation.y < count;
         ++idY)
    {
      chunks.add(start + glm::ivec2(idX, idY) * static_cast<int>(size), size);
    }
  }

  cellify(
    chunks,
    start + glm::ivec2(size) * static_cast<int>(RING_WIDTH) + compensation * static_cast<int>(size),
    center,
    size / 2,
    (count - RING_WIDTH * 2) * 2,
    subdivisions - 1);
}

std::vector<WorldRenderer::Chunk> WorldRenderer::generateChunkMap(
  const glm::vec2& true_origin) const
{
  ChunkArray chunks;
  unsigned maxSize = 64;
  unsigned largeChunkCount = 19;
  glm::ivec2 origin = glm::floor(true_origin / static_cast<float>(maxSize));

  cellify(
    chunks,
    origin * static_cast<int>(maxSize) - static_cast<int>(largeChunkCount * maxSize / 2),
    true_origin,
    maxSize,
    largeChunkCount,
    3);

  return chunks.chunks;
}

uint32_t WorldRenderer::mapChunks(const glm::vec2& camera_pos)
{
  auto chunks = generateChunkMap(camera_pos);
  chunkAllocator.genBindings(*this, chunks);
  std::memcpy(chunkMap.data(), chunks.data(), sizeof(Chunk) * chunks.size());
  return static_cast<uint32_t>(chunks.size());
}
