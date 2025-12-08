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

  sampler = etna::Sampler(etna::Sampler::CreateInfo{
    .filter = vk::Filter::eLinear,
    .addressMode = vk::SamplerAddressMode::eRepeat,
    .name = "default_sampler"});

  initLights();
  initGBuffers();
}

void WorldRenderer::loadScene(std::filesystem::path path)
{
  // sceneMgr->selectScene(path);
  sceneMgr->selectCompressedScene(path);
}

void WorldRenderer::loadShaders()
{
  etna::create_program(
    "static_mesh_material",
    {DEFERRED_SHADERS_ROOT "static_mesh.frag.spv", DEFERRED_SHADERS_ROOT "static_mesh.vert.spv"});
  etna::create_program("static_mesh", {DEFERRED_SHADERS_ROOT "static_mesh.vert.spv"});
  etna::create_program(
    "deferred_lights",
    {DEFERRED_SHADERS_ROOT "deferred_point_lights.frag.spv",
     DEFERRED_SHADERS_ROOT "fullscreen.vert.spv"});
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
      .blendingConfig =
        {
          .attachments =
            {{
               .blendEnable = vk::False,
               .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
             },
             {
               .blendEnable = vk::False,
               .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
             },
             {
               .blendEnable = vk::False,
               .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                 vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
             }},
          .logicOp = {},
        },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats =
            {
              geomBuffers.albedo.getFormat(),
              geomBuffers.normal.getFormat(),
              geomBuffers.depth.getFormat(),
            },
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });

  deferredLightingPipeline = {};
  deferredLightingPipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "deferred_lights",
    etna::GraphicsPipeline::CreateInfo{
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
    worldView = packet.mainCam.viewTm();
    worldInvProj = glm::inverse(packet.mainCam.projTm(aspect));
    worldViewProj = packet.mainCam.projTm(aspect) * packet.mainCam.viewTm();
  }
}

void WorldRenderer::renderScene(
  vk::CommandBuffer cmd_buf, const glm::mat4x4& glob_tm, vk::PipelineLayout pipeline_layout)
{
  if (!sceneMgr->getVertexBuffer())
    return;

  cmd_buf.bindVertexBuffers(0, {sceneMgr->getVertexBuffer()}, {0});
  cmd_buf.bindIndexBuffer(sceneMgr->getIndexBuffer(), 0, vk::IndexType::eUint32);

  pushConst2M.projView = glob_tm;

  auto instanceMeshes = sceneMgr->getInstanceMeshes();
  auto instanceMatrices = sceneMgr->getInstanceMatrices();

  auto meshes = sceneMgr->getMeshes();
  auto relems = sceneMgr->getRenderElements();

  for (std::size_t instIdx = 0; instIdx < instanceMeshes.size(); ++instIdx)
  {
    pushConst2M.model = instanceMatrices[instIdx];

    cmd_buf.pushConstants<PushConstants>(
      pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, {pushConst2M});

    const auto meshIdx = instanceMeshes[instIdx];

    for (std::size_t j = 0; j < meshes[meshIdx].relemCount; ++j)
    {
      const auto relemIdx = meshes[meshIdx].firstRelem + j;
      const auto& relem = relems[relemIdx];
      cmd_buf.drawIndexed(relem.indexCount, 1, relem.indexOffset, relem.vertexOffset, 0);
    }
  }
}

void WorldRenderer::initLights()
{
  auto& ctx = etna::get_context();

  std::vector<PointLight> lights = genPointLights();

  deferredPushConst2M.numberOfLights = static_cast<unsigned>(lights.size());

  pointLights = ctx.createBuffer(etna::Buffer::CreateInfo{
    .size = lights.size() * sizeof(PointLight),
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "lightsBuffer",
  });
  pointLights.map();

  std::memcpy(pointLights.data(), lights.data(), lights.size() * sizeof(PointLight));
}

void WorldRenderer::initGBuffers()
{
  auto& ctx = etna::get_context();

  geomBuffers.albedo = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "gbuffer_albedo",
    .format = vk::Format::eB8G8R8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  });

  geomBuffers.normal = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "gbuffer_normalDepth",
    .format = vk::Format::eB8G8R8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  });

  geomBuffers.depth = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "gbuffer_depth",
    .format = vk::Format::eR32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  });
}

void WorldRenderer::renderToGBuffers(vk::CommandBuffer cmd_buf)
{
  etna::set_state(
    cmd_buf,
    geomBuffers.albedo.get(),
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageAspectFlagBits::eColor);

  etna::set_state(
    cmd_buf,
    geomBuffers.normal.get(),
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageAspectFlagBits::eColor);

  etna::set_state(
    cmd_buf,
    geomBuffers.depth.get(),
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::flush_barriers(cmd_buf);

  {
    ETNA_PROFILE_GPU(cmd_buf, renderForward);

    etna::RenderTargetState renderTargets(
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      {
        {.image = geomBuffers.albedo.get(), .view = geomBuffers.albedo.getView({})},
        {.image = geomBuffers.normal.get(), .view = geomBuffers.normal.getView({})},
        {.image = geomBuffers.depth.get(), .view = geomBuffers.depth.getView({})},
      },
      {.image = mainViewDepth.get(), .view = mainViewDepth.getView({})});

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, staticMeshPipeline.getVkPipeline());
    renderScene(cmd_buf, worldViewProj, staticMeshPipeline.getVkPipelineLayout());
  }
}

void WorldRenderer::applyLighting(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  deferredPushConst2M.invProj = worldInvProj;
  deferredPushConst2M.view = worldView;

  etna::set_state(
    cmd_buf,
    geomBuffers.albedo.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);

  etna::set_state(
    cmd_buf,
    geomBuffers.normal.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);

  etna::set_state(
    cmd_buf,
    geomBuffers.depth.get(),
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::AccessFlagBits2::eShaderRead,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageAspectFlagBits::eColor);
  etna::flush_barriers(cmd_buf);

  {
    ETNA_PROFILE_GPU(cmd_buf, renderForward);

    etna::RenderTargetState renderTargets(
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      {{.image = target_image, .view = target_image_view}},
      {.image = mainViewDepth.get(), .view = mainViewDepth.getView({})});

    auto toyBasicInfo = etna::get_shader_program("deferred_lights");
    auto set = etna::create_descriptor_set(
      toyBasicInfo.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, pointLights.genBinding()},
        etna::Binding{
          1, geomBuffers.albedo.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{
          2, geomBuffers.normal.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{
          3, geomBuffers.depth.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      });

    vk::DescriptorSet vkSet = set.getVkSet();

    cmd_buf.bindPipeline(
      vk::PipelineBindPoint::eGraphics, deferredLightingPipeline.getVkPipeline());

    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      deferredLightingPipeline.getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);

    cmd_buf.pushConstants<DeferredPushConstants>(
      deferredLightingPipeline.getVkPipelineLayout(),
      vk::ShaderStageFlagBits::eFragment,
      0,
      {deferredPushConst2M});

    cmd_buf.draw(3, 1, 0, 0);
  }
}

static float random_float(float min, float max)
{
  if (min >= max)
    return min;
  return min + (max - min) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

std::vector<WorldRenderer::PointLight> WorldRenderer::genPointLights()
{
  std::vector<PointLight> result;

  for (unsigned lightIdx = 0; lightIdx < 100; ++lightIdx)
  {
    PointLight light;
    light.position =
      glm::vec3(random_float(-10.0f, 10.0f), random_float(0.4f, 3.0f), random_float(-10.0f, 10.0f));
    light.radius = random_float(2.0f, 5.0f);
    light.color =
      glm::vec3(random_float(0.2f, 1.0f), random_float(0.2f, 1.0f), random_float(0.2f, 1.0f));
    result.push_back(light);
  }


  return result;
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderWorld);

  renderToGBuffers(cmd_buf);
  applyLighting(cmd_buf, target_image, target_image_view);
}
