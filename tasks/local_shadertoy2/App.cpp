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
    {LOCAL_SHADERTOY2_SHADERS_ROOT "toy.frag.spv", LOCAL_SHADERTOY2_SHADERS_ROOT "toy.vert.spv"});

  etna::create_program(
    "intermediate",
    {LOCAL_SHADERTOY2_SHADERS_ROOT "intermediate.frag.spv",
     LOCAL_SHADERTOY2_SHADERS_ROOT "toy.vert.spv"});

  mainPipeline = {};
  mainPipeline = etna::get_context().getPipelineManager().createGraphicsPipeline(
    "toy_basic",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {vkWindow->getCurrentFormat()},
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

  ballTexture = etna::get_context().createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{BALL_TEXTURE_RESOLUTION.x, BALL_TEXTURE_RESOLUTION.y, 1},
    .name = "ballTexture",
    .format = vk::Format::eB8G8R8A8Srgb,
    .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
  });

  sampler = etna::Sampler(etna::Sampler::CreateInfo{
    .filter = vk::Filter::eLinear,
    .addressMode = vk::SamplerAddressMode::eRepeat,
    .name = "default_sampler"});

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
        backbuffer,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(currentCmdBuf);

      {
        etna::RenderTargetState state{
          currentCmdBuf, {{}, {resolution.x, resolution.y}}, {{backbuffer, backbufferView}}, {}};

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
