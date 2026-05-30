#include "Scene.h"

#include "imgui.h"
#include <vector>

void Scene::initialize()
{
  deferred.resolution(getResolution()).init(&getCmdBuf());
  reflectionDeferred.resolution(getResolution()).init(&getCmdBuf());
  bindless.init(*this);

  ssao.init(*this);

  aliasedScene.name("preFXAA")
    .size(getResolution())
    .format(vk::Format::eB8G8R8A8Unorm)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  reflectionWithLighting.name("litReflection")
    .size(getResolution())
    .format(vk::Format::eB8G8R8A8Unorm)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  directLight.name("directLighting")
    .size(getResolution().x, getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  reflectionDirectLight.name("reflectionDirectLighting")
    .size(getResolution().x, getResolution().y)
    .format(vk::Format::eR16Sfloat)
    .useColorAttachment()
    .useSampled()
    .init(&getCmdBuf());

  shadowMap.init(*this);

  directLightingShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/direct_light.frag.spv")
    .addColorAttachment(directLight.getFormat());
  for (unsigned smapIdx = 0; smapIdx < ShadowMap::NUM_LEVELS; ++smapIdx)
  {
    directLightingShader.addPersistentBinding(shadowMap.getTexture(smapIdx), getDefaultSampler());
  }
  directLightingShader.init();

  fog.init(*this, shadowMap);

  lightMixer
    .shaderPath(EVERYTHING_SHADERS_ROOT "/combine_lighting.frag.spv")
    .addColorAttachment(aliasedScene.getFormat())
    .init();

  fxaaShader
    .shaderPath(EVERYTHING_SHADERS_ROOT "/fxaa.frag.spv")
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm)
    .init();

  skySphere = spg::Texture::loadFromPng(TEXTURES_ROOT "/qwantani_noon_puresky_2k.png", getCmdBuf());
  skySphereBlurry = spg::Texture::loadFromPng(TEXTURES_ROOT "/qwantani_noon_puresky_2k_blurry.png", getCmdBuf());

  subsurface.init(*this);

  std::vector<Emitter> emitters;
  emitters.reserve(2);

  Emitter orangeEmitter;
  orangeEmitter.particleTemplate.px = 0.0f;
  orangeEmitter.particleTemplate.py = 0.2f;
  orangeEmitter.particleTemplate.pz = 0.0f;
  orangeEmitter.particleTemplate.vx = 0.0f;
  orangeEmitter.particleTemplate.vy = 2.5f;
  orangeEmitter.particleTemplate.vz = 0.0f;
  orangeEmitter.particleTemplate.ttl = 2.0f;
  orangeEmitter.particleTemplate.size = 0.2f;
  orangeEmitter.particleTemplate.colorRGBA = packColorRGBA(glm::u8vec3(255, 120, 30), 180);
  orangeEmitter.positionVariation = 0.25f;
  orangeEmitter.velocityVariation = 0.6f;
  orangeEmitter.sizeVariation = 0.05f;
  orangeEmitter.transparencyVariation = 40.0f;
  orangeEmitter.ttlVariation = 0.5f;
  orangeEmitter.spawnDt = 0.02f;
  emitters.push_back(orangeEmitter);

  Emitter blueEmitter;
  blueEmitter.particleTemplate.px = 2.0f;
  blueEmitter.particleTemplate.py = 0.3f;
  blueEmitter.particleTemplate.pz = -1.5f;
  blueEmitter.particleTemplate.vx = 0.0f;
  blueEmitter.particleTemplate.vy = 0.8f;
  blueEmitter.particleTemplate.vz = 0.4f;
  blueEmitter.particleTemplate.ttl = 3.5f;
  blueEmitter.particleTemplate.size = 0.3f;
  blueEmitter.particleTemplate.colorRGBA = packColorRGBA(glm::u8vec3(80, 140, 255), 110);
  blueEmitter.positionVariation = 0.6f;
  blueEmitter.velocityVariation = 0.35f;
  blueEmitter.sizeVariation = 0.08f;
  blueEmitter.transparencyVariation = 30.0f;
  blueEmitter.ttlVariation = 1.0f;
  blueEmitter.spawnDt = 0.05f;
  emitters.push_back(blueEmitter);

  particles.init(*this, emitters, aliasedScene.getFormat(), deferred.depth.getFormat());

  fullWhite = spg::Texture::loadFromPng(TEXTURES_ROOT "/white_pixel.png", getCmdBuf());
  fullBlack = spg::Texture::loadFromPng(TEXTURES_ROOT "/black_pixel.png", getCmdBuf());

  water.init(*this, deferred.depth.getFormat());
}

void Scene::render()
{
  struct CombinedMatrices
  {
    glm::mat4 invProjView;
    glm::mat4 lightProjView;
    glm::mat4 invView;
    glm::mat4 lightInvView;
  };

  bindless.render(*this, deferred);

  shadowMap.updateCameraPositions(getCam().position);
  bindless.renderShadowMap(*this, shadowMap);

  Camera reflectionCam = getCam();
  const glm::vec3 camForward = getCam().forward();
  const glm::vec3 camUp = getCam().up();
  reflectionCam.position.y = 2.0f * waterLevel - reflectionCam.position.y;
  const glm::vec3 reflectedForward = glm::vec3(camForward.x, -camForward.y, camForward.z);
  const glm::vec3 reflectedUp = glm::vec3(camUp.x, -camUp.y, camUp.z);
  reflectionCam.lookAt(reflectionCam.position, reflectionCam.position + reflectedForward, reflectedUp);

  overrideCamera(reflectionCam);
  bindless.renderWithCutoff(*this, reflectionDeferred, waterLevel);
  clearCameraOverride();

  CombinedMatrices reflectedMatrices;
  reflectedMatrices.invProjView = glm::inverse(reflectionCam.projTm(getAspect()) * reflectionCam.viewTm());
  Camera& shadowCam = shadowMap.getCamera(0);
  reflectedMatrices.lightProjView = shadowCam.projTm(1.0f) * shadowCam.viewTm();
  reflectedMatrices.invView = glm::inverse(reflectionCam.viewTm());
  reflectedMatrices.lightInvView = glm::inverse(shadowCam.viewTm());

  directLightingShader.dispatch(getCmdBuf())
    .bind(0, reflectionDeferred.depth, getDefaultSampler())
    .bind(1, reflectionDeferred.normalEmissive, getDefaultSampler())
    .push(reflectedMatrices)
    .attach(reflectionDirectLight);

  CombinedMatrices mainMatrices;
  Camera& shadowCamMain = shadowMap.getCamera(0);
  mainMatrices.invProjView = glm::inverse(getWorldViewProj());
  mainMatrices.lightProjView = shadowCamMain.projTm(1.0f) * shadowCamMain.viewTm();
  mainMatrices.invView = glm::inverse(getWorldView());
  mainMatrices.lightInvView = glm::inverse(shadowCamMain.viewTm());

  directLightingShader.dispatch(getCmdBuf())
    .bind(0, deferred.depth, getDefaultSampler())
    .bind(1, deferred.normalEmissive, getDefaultSampler())
    .push(mainMatrices)
    .attach(directLight);

  fog.render(*this, shadowMap, deferred.depth);

  if (enableSSAO)
  {
    ssao.render(*this, deferred.depth, deferred.normalEmissive);
  }

  struct LightMixerParams
  {
    glm::mat4 invProjView;
    glm::vec4 cameraPos;
    uint32_t enableSSAO;
    uint32_t enableSSSS;
  };
  LightMixerParams lightMixerParams;
  lightMixerParams.invProjView = glm::inverse(reflectionCam.projTm(getAspect()) * reflectionCam.viewTm());
  lightMixerParams.cameraPos = glm::vec4(reflectionCam.position, 1.0f);
  lightMixerParams.enableSSAO = false;
  lightMixerParams.enableSSSS = false;

  lightMixer.dispatch(getCmdBuf())
    .bind(0, reflectionDeferred.albedo, getDefaultSampler())
    .bind(1, fullWhite, getDefaultSampler())
    .bind(2, reflectionDirectLight, getDefaultSampler())
    .bind(3, fullBlack, getDefaultSampler())
    .bind(4, skySphere, getDefaultSampler())
    .bind(5, reflectionDeferred.depth, getDefaultSampler())
    .bind(6, skySphereBlurry, getDefaultSampler())
    .bind(7, fullBlack, getDefaultSampler())
    .push(lightMixerParams)
    .attach(reflectionWithLighting);

  lightMixerParams.invProjView = glm::inverse(getWorldViewProj());
  lightMixerParams.cameraPos = glm::vec4(getCam().position, 1.0f);
  lightMixerParams.enableSSAO = static_cast<uint32_t>(enableSSAO);
  lightMixerParams.enableSSSS = static_cast<uint32_t>(enableSSSS);

  if (enableSSSS)
  {
    subsurface.render(*this, directLight, deferred.depth, deferred.normalEmissive);
  }

  lightMixer.dispatch(getCmdBuf())
    .bind(0, deferred.albedo, getDefaultSampler())
    .bind(1, ssao.getAo(), getDefaultSampler())
    .bind(2, directLight, getDefaultSampler())
    .bind(3, fog.getTexture(), getDefaultSampler())
    .bind(4, skySphere, getDefaultSampler())
    .bind(5, deferred.depth, getDefaultSampler())
    .bind(6, skySphereBlurry, getDefaultSampler())
    .bind(7, subsurface.getSubsurface(), getDefaultSampler())
    .push(lightMixerParams)
    .attach(aliasedScene);

  if (enableParticles)
  {
    particles.tick(*this, getDeltaTime());
  }

  spg::Texture* postWaterColor = &aliasedScene;
  if (enableWater)
  {
    water.render(*this, waterLevel, aliasedScene, deferred.depth, reflectionWithLighting);
    postWaterColor = &water.getTexture();
  }

  if (enableParticles)
  {
    particles.draw(*this, *postWaterColor, deferred.depth);
  }

  struct FxaaParams
  {
    uint32_t enableFXAA;
  } fxaaParams{enableFXAA};

  fxaaShader.dispatch(getCmdBuf())
    .bind(0, *postWaterColor, getDefaultSampler())
    .push(fxaaParams)
    .attach(getScreenAttachment(), getResolution());
}

void Scene::renderGui()
{
  ImGui::Checkbox("Enable FXAA", &enableFXAA);
  ImGui::Checkbox("Enable SSAO", &enableSSAO);
  ImGui::Checkbox("Enable SSSS", &enableSSSS);
  ImGui::Checkbox("Enable Particles", &enableParticles);
  ImGui::Checkbox("Enable Water", &enableWater);
}
