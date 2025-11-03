#include "WorldRenderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>


WorldRenderer::WorldRenderer()
  : sceneMgr{std::make_unique<SceneManager>()}
{
}

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

  constants = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = sizeof(glm::mat4) * INSTANCES_PER_CALL,
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
    .name = "constants0",
  });
  constants.map();
}

void WorldRenderer::loadScene(std::filesystem::path path)
{
  // sceneMgr->selectScene(path);
  sceneMgr->selectCompressedScene(path);

  for (size_t instanceIdx = 0; instanceIdx < sceneMgr->getInstanceMatrices().size(); instanceIdx++)
  {
    auto meshIdx = sceneMgr->getInstanceMeshes()[instanceIdx];
    meshInstancingMap.try_emplace(meshIdx);
    meshInstancingMap[meshIdx].emplace_back(instanceIdx);
  }
}

void WorldRenderer::loadShaders()
{
  etna::create_program(
    "static_mesh_material",
    {MANY_OBJECTS_SHADERS_ROOT "static_mesh.frag.spv",
     MANY_OBJECTS_SHADERS_ROOT "static_mesh.vert.spv"});
  etna::create_program("static_mesh", {MANY_OBJECTS_SHADERS_ROOT "static_mesh.vert.spv"});
}

void WorldRenderer::setupPipelines(vk::Format swapchain_format)
{
  etna::VertexShaderInputDescription sceneVertexInputDesc{
    .bindings = {etna::VertexShaderInputDescription::Binding{
      .byteStreamDescription = sceneMgr->getCompressedVertexFormatDescription(),
    }},
  };

  auto& pipelineManager = etna::get_context().getPipelineManager();

  staticMeshPipeline = {};
  staticMeshPipeline = pipelineManager.createGraphicsPipeline(
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
  }
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderWorld);

  for (const auto& [meshIdx, instances] : meshInstancingMap)
  {
    std::vector<glm::mat4> instanceMatrices;
    for (size_t instanceIdx = 0; instanceIdx < instances.size(); ++instanceIdx)
    {
      auto matrixIdx = instances[instanceIdx];
      instanceMatrices.emplace_back(sceneMgr->getInstanceMatrices()[matrixIdx]);
    }

    std::memcpy(
      constants.data(), instanceMatrices.data(), sizeof(glm::mat4) * instanceMatrices.size());

    {
      ETNA_PROFILE_GPU(cmd_buf, renderForward);

      etna::RenderTargetState renderTargets(
        cmd_buf,
        {{0, 0}, {resolution.x, resolution.y}},
        {{.image = target_image, .view = target_image_view}},
        {.image = mainViewDepth.get(), .view = mainViewDepth.getView({})});

      auto intermediateInfo = etna::get_shader_program("static_mesh_material");
      auto set = etna::create_descriptor_set(
        intermediateInfo.getDescriptorLayoutId(0),
        cmd_buf,
        {etna::Binding{0, constants.genBinding()}});

      vk::DescriptorSet vkSet = set.getVkSet();

      cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, staticMeshPipeline.getVkPipeline());

      cmd_buf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        staticMeshPipeline.getVkPipelineLayout(),
        0,
        1,
        &vkSet,
        0,
        nullptr);

      cmd_buf.bindVertexBuffers(0, {sceneMgr->getVertexBuffer()}, {0});
      cmd_buf.bindIndexBuffer(sceneMgr->getIndexBuffer(), 0, vk::IndexType::eUint32);

      pushConst2M.projView = worldViewProj;

      cmd_buf.pushConstants<PushConstants>(
        staticMeshPipeline.getVkPipelineLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        {pushConst2M});

      for (std::size_t j = 0; j < sceneMgr->getMeshes()[meshIdx].relemCount; ++j)
      {
        const auto relemIdx = sceneMgr->getMeshes()[meshIdx].firstRelem + j;
        const auto& relem = sceneMgr->getRenderElements()[relemIdx];
        cmd_buf.drawIndexed(
          relem.indexCount,
          static_cast<uint32_t>(instanceMatrices.size()),
          relem.indexOffset,
          relem.vertexOffset,
          0);
      }
    }

    etna::flush_barriers(cmd_buf);
  }
}
