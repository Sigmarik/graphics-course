#include "App.hpp"

#include "etna/RenderTargetStates.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <etna/Etna.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>


App::App()
  : resolution{1280, 720}
  , useVsync{true}
{
  // First, we need to initialize Vulkan, which is not trivial because
  // extensions are required for just about anything.
  {
    // GLFW tells us which extensions it needs to present frames to the OS window.
    // Actually rendering anything to a screen is optional in Vulkan, you can
    // alternatively save rendered frames into files, send them over network, etc.
    // Instance extensions do not depend on the actual GPU, only on the OS.
    auto glfwInstExts = windowing.getRequiredVulkanInstanceExtensions();

    std::vector<const char*> instanceExtensions{glfwInstExts.begin(), glfwInstExts.end()};

    // We also need the swapchain device extension to get access to the OS
    // window from inside of Vulkan on the GPU.
    // Device extensions require HW support from the GPU.
    // Generally, in Vulkan, we call the GPU a "device" and the CPU/OS combination a "host."
    std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Etna does all the Vulkan initialization heavy lifting.
    // You can skip figuring out how it works for now.
    etna::initialize(etna::InitParams{
      .applicationName = "Local Shadertoy",
      .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
      .instanceExtensions = instanceExtensions,
      .deviceExtensions = deviceExtensions,
      // Replace with an index if etna detects your preferred GPU incorrectly
      .physicalDeviceIndexOverride = {},
      .numFramesInFlight = 1,
    });
  }

  // Now we can create an OS window
  osWindow = windowing.createWindow(OsWindow::CreateInfo{
    .resolution = resolution,
  });

  tonemappingParams.avg = 0.7f;
  tonemappingParams.var = std::max(tonemappingParams.avg, 0.1f);
  tonemappingParams.resolutionX = resolution.x;
  tonemappingParams.resolutionY = resolution.y;

  // But we also need to hook the OS window up to Vulkan manually!
  {
    // First, we ask GLFW to provide a "surface" for the window,
    // which is an opaque description of the area where we can actually render.
    auto surface = osWindow->createVkSurface(etna::get_context().getInstance());

    // Then we pass it to Etna to do the complicated work for us
    vkWindow = etna::get_context().createWindow(etna::Window::CreateInfo{
      .surface = std::move(surface),
    });

    // And finally ask Etna to create the actual swapchain so that we can
    // get (different) images each frame to render stuff into.
    // Here, we do not support window resizing, so we only need to call this once.
    auto [w, h] = vkWindow->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });

    // Technically, Vulkan might fail to initialize a swapchain with the requested
    // resolution and pick a different one. This, however, does not occur on platforms
    // we support. Still, it's better to follow the "intended" path.
    resolution = {w, h};
  }

  // Next, we need a magical Etna helper to send commands to the GPU.
  // How it is actually performed is not trivial, but we can skip this for now.
  commandManager = etna::get_context().createPerFrameCmdMgr();


  etna::create_program(
    "toy_basic",
    {TONEMAPPING_SHADERS_ROOT "toy.frag.spv", TONEMAPPING_SHADERS_ROOT "toy.vert.spv"});

  etna::create_program(
    "toy_tonemap",
    {TONEMAPPING_SHADERS_ROOT "tonemap.frag.spv", TONEMAPPING_SHADERS_ROOT "toy.vert.spv"});

  etna::create_program("toy_avg", {TONEMAPPING_SHADERS_ROOT "avg.comp.spv"});

  etna::create_program(
    "intermediate",
    {TONEMAPPING_SHADERS_ROOT "intermediate.frag.spv", TONEMAPPING_SHADERS_ROOT "toy.vert.spv"});

  toneMappingPipeline = {};
  toneMappingPipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "toy_tonemap",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vkWindow->getCurrentFormat()},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });

  mainPipeline = {};
  mainPipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "toy_basic",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vk::Format::eB10G11R11UfloatPack32},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });

  intermediatePipeline = {};
  intermediatePipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "intermediate",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vk::Format::eB8G8R8A8Srgb},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });

  avgBrightnessPipeline =
    etna::get_context().getPipelineManager().createComputePipeline("toy_avg", {});

  ballTexture = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{BALL_TEXTURE_RESOLUTION.x, BALL_TEXTURE_RESOLUTION.y, 1},
    .name = "ballTexture",
    .format = vk::Format::eB8G8R8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  });

  hdrFrameCopy = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "HDR frame copy",
    .format = vk::Format::eB10G11R11UfloatPack32,
    .imageUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
  });

  hdrFrame = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "HDR frame",
    .format = vk::Format::eB10G11R11UfloatPack32,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |
      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled,
  });

  hdrFrameBackBuf = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "HDR frame backbuffer",
    .format = vk::Format::eB10G11R11UfloatPack32,
    .imageUsage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc |
      vk::ImageUsageFlagBits::eSampled,
  });

  sampler = etna::Sampler(etna::Sampler::CreateInfo{
    .filter = vk::Filter::eLinear,
    .addressMode = vk::SamplerAddressMode::eRepeat,
    .name = "default_sampler"});

  transferHelper =
    std::make_unique<etna::BlockingTransferHelper>(etna::BlockingTransferHelper::CreateInfo{
      .stagingSize = sizeof(uint32_t),
    });

  brightnessReadbackBuffer = etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = sizeof(uint32_t),
    .bufferUsage = vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU,
    .name = "brightnessReadbackBuffer",
  });
  brightnessReadbackBuffer.map();

  params.resolutionX = resolution.x;
  params.resolutionY = resolution.y;
  params.mouseX = params.mouseY = 0.0f;
  params.time = 0.0f;

  startTime = std::chrono::steady_clock::now();
}

App::~App()
{
  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::run()
{
  importTextures();

  while (!osWindow->isBeingClosed())
  {
    windowing.poll();

    if (is_held_down(osWindow->keyboard.keys[static_cast<int>(KeyboardKey::kR)]))
    {
      etna::reload_shaders();
      startTime = std::chrono::steady_clock::now();
    }

    updateParams();

    drawFrame();
  }

  // We need to wait for the GPU to execute the last frame before destroying
  // all resources and closing the application.
  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}

void App::drawFrame()
{
  // First, get a command buffer to write GPU commands into.
  auto currentCmdBuf = commandManager->acquireNext();

  // Next, tell Etna that we are going to start processing the next frame.
  etna::begin_frame();

  // And now get the image we should be rendering the picture into.
  auto nextSwapchainImage = vkWindow->acquireNext();

  // When window is minimized, we can't render anything in Windows
  // because it kills the swapchain, so we skip frames in this case.
  if (nextSwapchainImage)
  {
    auto [backbuffer, backbufferView, backbufferAvailableSem, backbufferReadyForPresentSem] =
      *nextSwapchainImage;

    ETNA_CHECK_VK_RESULT(currentCmdBuf.begin(vk::CommandBufferBeginInfo{}));
    {
      etna::set_state(
        currentCmdBuf,
        ballTexture.get(),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(currentCmdBuf);

      {
        etna::RenderTargetState state{
          currentCmdBuf,
          {{}, {BALL_TEXTURE_RESOLUTION.x, BALL_TEXTURE_RESOLUTION.y}},
          {{ballTexture.get(), ballTexture.getView({})}},
          {}};

        currentCmdBuf.bindPipeline(
          vk::PipelineBindPoint::eGraphics, intermediatePipeline.getVkPipeline());

        currentCmdBuf.pushConstants(
          intermediatePipeline.getVkPipelineLayout(),
          vk::ShaderStageFlagBits::eFragment,
          0,
          sizeof(params),
          &params);

        currentCmdBuf.draw(3, 1, 0, 0);
      }
      etna::flush_barriers(currentCmdBuf);

      etna::set_state(
        currentCmdBuf,
        ballTexture.get(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eColorAttachmentRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::set_state(
        currentCmdBuf,
        skyTexture.get(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eColorAttachmentRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::set_state(
        currentCmdBuf,
        hdrFrame.get(),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(currentCmdBuf);

      {
        etna::RenderTargetState state{
          currentCmdBuf,
          {{}, {resolution.x, resolution.y}},
          {{hdrFrame.get(), hdrFrame.getView({})}},
          {}};

        auto toyBasicInfo = etna::get_shader_program("toy_basic");
        auto set = etna::create_descriptor_set(
          toyBasicInfo.getDescriptorLayoutId(0),
          currentCmdBuf,
          {etna::Binding{
             0, ballTexture.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
           etna::Binding{
             1, skyTexture.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}});

        vk::DescriptorSet vkSet = set.getVkSet();

        currentCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mainPipeline.getVkPipeline());

        currentCmdBuf.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          mainPipeline.getVkPipelineLayout(),
          0,
          1,
          &vkSet,
          0,
          nullptr);

        currentCmdBuf.pushConstants(
          mainPipeline.getVkPipelineLayout(),
          vk::ShaderStageFlagBits::eFragment,
          0,
          sizeof(params),
          &params);

        currentCmdBuf.draw(3, 1, 0, 0);
      }

      etna::set_state(
        currentCmdBuf,
        hdrFrame.get(),
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::set_state(
        currentCmdBuf,
        hdrFrameCopy.get(),
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(currentCmdBuf);

      constexpr auto kSubresurce =
        vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1};
      const vk::ArrayWrapper1D<vk::Offset3D, 2UL> kOffsets = {
        {vk::Offset3D{0, 0, 0},
         vk::Offset3D{static_cast<int32_t>(resolution.x), static_cast<int32_t>(resolution.y), 1}}};
      const vk::ImageBlit kRegion = {
        .srcSubresource = kSubresurce,
        .srcOffsets = kOffsets,
        .dstSubresource = kSubresurce,
        .dstOffsets = kOffsets,
      };
      currentCmdBuf.blitImage(
        hdrFrame.get(),
        vk::ImageLayout::eTransferSrcOptimal,
        hdrFrameCopy.get(),
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &kRegion,
        vk::Filter::eLinear);

      approximateBrightnessDistribution(currentCmdBuf);

      etna::set_state(
        currentCmdBuf,
        hdrFrameCopy.get(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eColorAttachmentRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::set_state(
        currentCmdBuf,
        backbuffer,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(currentCmdBuf);

      {
        etna::RenderTargetState state{
          currentCmdBuf, {{}, {resolution.x, resolution.y}}, {{backbuffer, backbufferView}}, {}};

        auto toyBasicInfo = etna::get_shader_program("toy_tonemap");
        auto set = etna::create_descriptor_set(
          toyBasicInfo.getDescriptorLayoutId(0),
          currentCmdBuf,
          {etna::Binding{
            0, hdrFrameCopy.genBinding(sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}});

        vk::DescriptorSet vkSet = set.getVkSet();

        currentCmdBuf.bindPipeline(
          vk::PipelineBindPoint::eGraphics, toneMappingPipeline.getVkPipeline());

        currentCmdBuf.bindDescriptorSets(
          vk::PipelineBindPoint::eGraphics,
          toneMappingPipeline.getVkPipelineLayout(),
          0,
          1,
          &vkSet,
          0,
          nullptr);

        currentCmdBuf.pushConstants(
          toneMappingPipeline.getVkPipelineLayout(),
          vk::ShaderStageFlagBits::eFragment,
          0,
          sizeof(tonemappingParams),
          &tonemappingParams);

        currentCmdBuf.draw(3, 1, 0, 0);
      }
      etna::flush_barriers(currentCmdBuf);

      // At the end of "rendering", we are required to change how the pixels of the
      // swapchain image are laid out in memory to something that is appropriate
      // for presenting to the window (while preserving the content of the pixels!).
      etna::set_state(
        currentCmdBuf,
        backbuffer,
        // This looks weird, but is correct. Ask about it later.
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageAspectFlagBits::eColor);
      // And of course flush the layout transition.
      etna::flush_barriers(currentCmdBuf);
    }
    ETNA_CHECK_VK_RESULT(currentCmdBuf.end());

    // We are done recording GPU commands now and we can send them to be executed by the GPU.
    // Note that the GPU won't start executing our commands before the backbufferAvailableSem
    // semaphore is signalled, which will happen when the OS says that the next swapchain image
    // is ready, and the result image will be ready for present after backbufferReadyForPresent
    // is signalled by GPU
    auto renderingDone = commandManager->submit(
      std::move(currentCmdBuf),
      std::move(backbufferAvailableSem),
      std::move(backbufferReadyForPresentSem));

    // Finally, present the backbuffer the screen, but only after the GPU tells the OS
    // that it is done executing the command buffer via the renderingDone semaphore.
    const bool presented = vkWindow->present(std::move(renderingDone), backbufferView);

    if (!presented)
      nextSwapchainImage = std::nullopt;
  }

  etna::end_frame();

  // After a window us un-minimized, we need to restore the swapchain to continue rendering.
  if (!nextSwapchainImage && osWindow->getResolution() != glm::uvec2{0, 0})
  {
    auto [w, h] = vkWindow->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });
    ETNA_VERIFY((resolution == glm::uvec2{w, h}));
  }
}

void App::updateParams()
{
  glm::vec2 mousePosition = osWindow.get()->mouse.freePos;
  params.time = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
  params.mouseX = mousePosition.x;
  params.mouseY = mousePosition.y;
}

void App::importTextures()
{
  auto cmdBuf = commandManager->acquireNext();

  int dimX = 0, dimY = 0;
  stbi_uc* bytes = stbi_load(TEXTURES_ROOT "cloudy_sky.png", &dimX, &dimY, nullptr, 4);
  assert(bytes && "Failed to load the sky sphere");

  etna::Image::CreateInfo fileTextureInfo{
    .extent = vk::Extent3D{static_cast<unsigned>(dimX), static_cast<unsigned>(dimY), 1},
    .name = "skySphere",
    .format = vk::Format::eR8G8B8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  };
  skyTexture = etna::create_image_from_bytes(fileTextureInfo, cmdBuf, bytes);

  stbi_image_free(bytes);
}

static float decode_11bit_float(uint32_t bits11)
{
  // 11-bit layout: 1 sign bit (implicitly 0 for unsigned), 5-bit exponent, 5-bit mantissa
  // bits11: [10:0] where bit10 = sign(assumed 0), bits[9:5]=exp (5 bits), bits[4:0]=mantissa (5
  // bits) We'll treat it as an unsigned 11-bit float with no sign bit: exponent bias = 15 (for
  // 5-bit exponent)
  const int exp = (bits11 >> 6) & 0x1F; // 5-bit exponent
  const int mant = bits11 & 0x3F;       // 5-bit mantissa

  // normalized: (1 + mant/32) * 2^(exp - bias)
  const float m = 1.0f + (float)mant / (1 << 6);
  const int e = exp - 15;
  return exp2f(static_cast<float>(e)) * m;
}

float get_red(uint32_t b10g11r11ufloatpack_pixel)
{
  const uint32_t r_bits = b10g11r11ufloatpack_pixel & 0x7FFu;
  return decode_11bit_float(r_bits);
}

void App::approximateBrightnessDistribution(vk::CommandBuffer& current_cmd_buf)
{
  averagingParams.resolutionX = resolution.x;
  averagingParams.resolutionY = resolution.y;

  bool useBackBuffer = true;
  for (unsigned shift = 1; shift < resolution.x; shift *= 2, useBackBuffer = !useBackBuffer)
  {
    averagingParams.shiftX = shift;
    averagingParams.shiftY = 0;

    auto toyInfo = etna::get_shader_program("toy_avg");
    const auto set = etna::create_descriptor_set(
      toyInfo.getDescriptorLayoutId(0),
      current_cmd_buf,
      {
        etna::Binding{
          useBackBuffer ? 0u : 1u, hdrFrame.genBinding(sampler.get(), vk::ImageLayout::eGeneral)},
        etna::Binding{
          useBackBuffer ? 1u : 0u,
          hdrFrameBackBuf.genBinding(sampler.get(), vk::ImageLayout::eGeneral)},
      });

    vk::DescriptorSet vkSet = set.getVkSet();

    current_cmd_buf.bindPipeline(
      vk::PipelineBindPoint::eCompute, avgBrightnessPipeline.getVkPipeline());
    current_cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      avgBrightnessPipeline.getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);

    current_cmd_buf.pushConstants(
      avgBrightnessPipeline.getVkPipelineLayout(),
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(averagingParams),
      &averagingParams);
    etna::flush_barriers(current_cmd_buf);

    current_cmd_buf.dispatch((resolution.x - shift + 31) / 32, (resolution.y + 31) / 32, 1);
  }

  for (unsigned shift = 1; shift < resolution.y; shift *= 2, useBackBuffer = !useBackBuffer)
  {
    averagingParams.shiftX = 0;
    averagingParams.shiftY = shift;

    auto toyInfo = etna::get_shader_program("toy_avg");
    const auto set = etna::create_descriptor_set(
      toyInfo.getDescriptorLayoutId(0),
      current_cmd_buf,
      {
        etna::Binding{
          useBackBuffer ? 0u : 1u, hdrFrame.genBinding(sampler.get(), vk::ImageLayout::eGeneral)},
        etna::Binding{
          useBackBuffer ? 1u : 0u,
          hdrFrameBackBuf.genBinding(sampler.get(), vk::ImageLayout::eGeneral)},
      });

    vk::DescriptorSet vkSet = set.getVkSet();

    current_cmd_buf.bindPipeline(
      vk::PipelineBindPoint::eCompute, avgBrightnessPipeline.getVkPipeline());
    current_cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      avgBrightnessPipeline.getVkPipelineLayout(),
      0,
      1,
      &vkSet,
      0,
      nullptr);

    current_cmd_buf.pushConstants(
      avgBrightnessPipeline.getVkPipelineLayout(),
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(averagingParams),
      &averagingParams);
    etna::flush_barriers(current_cmd_buf);

    current_cmd_buf.dispatch(1, (resolution.y - shift + 31) / 32, 1);
  }

  etna::Image& readbackImage = useBackBuffer ? hdrFrameBackBuf : hdrFrameBackBuf;

  vk::Extent3D extent = {1, 1, 1};
  const vk::ImageSubresourceLayers subresource{
    .aspectMask = vk::ImageAspectFlagBits::eColor,
    .mipLevel = 0,
    .baseArrayLayer = 0,
    .layerCount = 1,
  };
  const vk::BufferImageCopy kRegion = {
    .bufferOffset = 0,
    .bufferRowLength = 1,
    .bufferImageHeight = 1,
    .imageSubresource = subresource,
    .imageOffset = vk::Offset3D{0, 0, 0},
    .imageExtent = extent,
  };

  current_cmd_buf.copyImageToBuffer(
    readbackImage.get(), vk::ImageLayout::eGeneral, brightnessReadbackBuffer.get(), 1u, &kRegion);

  uint32_t pixelData = *reinterpret_cast<const uint32_t*>(brightnessReadbackBuffer.data());
  tonemappingParams.avg = tonemappingParams.avg * 0.99f + get_red(pixelData) * 0.01f;
  tonemappingParams.var = std::max(tonemappingParams.avg, 0.1f);
  tonemappingParams.resolutionX = resolution.x;
  tonemappingParams.resolutionY = resolution.y;
}
