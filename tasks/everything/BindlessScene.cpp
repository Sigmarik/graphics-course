#include "BindlessScene.h"

struct InstanceInfo
{
  glm::mat4 transform;
  uint32_t textureIdx;
  uint32_t _pad[3];
  glm::vec3 fallbackDiffuse;
  uint32_t _pad2[1];
};

void BindlessScene::init(spg::App& app)
{
  sceneManager.selectCompressedScene(GRAPHICS_COURSE_RESOURCES_ROOT "/scenes/low_poly_dark_town/scene_baked.gltf");

  auto instanceMeshes = sceneManager.getInstanceMeshes();
  auto instanceMatrices = sceneManager.getInstanceMatrices();
  auto meshes = sceneManager.getMeshes();
  auto relems = sceneManager.getRenderElements();

  std::vector<vk::DrawIndexedIndirectCommand> drawCommands;
  std::vector<InstanceInfo> instances;

  // Man do I LOVE fucking with GLFT models! Their standard is SO NICE,
  // I totally do not need to bend over backwards and spread my ass cheeks to
  // do the simplest of procedures with the thing.
  for (size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx)
  {
    std::vector<size_t> instanceIndices;
    for (size_t idx = 0; idx < instanceMeshes.size(); ++idx)
    {
      if (instanceMeshes[idx] == meshIdx) instanceIndices.push_back(idx);
    }

    for (size_t relemIdx = meshes[meshIdx].firstRelem;
      relemIdx < meshes[meshIdx].firstRelem + meshes[meshIdx].relemCount; ++relemIdx)
    {
      const auto& relem = relems[relemIdx];
      drawCommands.push_back(vk::DrawIndexedIndirectCommand{
        .indexCount = relem.indexCount,
        .instanceCount = static_cast<uint32_t>(instanceIndices.size()),
        .firstIndex = relem.indexOffset,
        .vertexOffset = static_cast<int32_t>(relem.vertexOffset),
        .firstInstance = static_cast<uint32_t>(instances.size()),
      });
      for (auto instanceIdx : instanceIndices)
      {
        std::ignore = instanceIdx;
        InstanceInfo inst;
        inst.transform = instanceMatrices[instanceIdx];
        inst.textureIdx = relem.albedoTextureIndex;
        inst.fallbackDiffuse = relem.fallbackDiffuse;
        instances.emplace_back(inst);
      }
    }
  }

  indirect.name("indirect")
    .useIndirect()
    .initAndCopy(drawCommands);

  indirectCount = static_cast<unsigned>(drawCommands.size());

  instanceInfo.name("instanceInfo")
    .useStorage()
    .initAndCopy(instances);

  shader.programName("indirectShader")
    .vertexPath(EVERYTHING_SHADERS_ROOT "indirect.vert.spv")
    .fragmentPath(EVERYTHING_SHADERS_ROOT "indirect.frag.spv")
    .vertexFormat(sceneManager.getCompressedVertexFormatDescription())
    .depthOutputFormat(DeferredTextureBunch::DEPTH_FORMAT)
    .addColorAttachment(DeferredTextureBunch::ALBEDO_FORMAT)
    .addColorAttachment(DeferredTextureBunch::NORMAL_EMISSIVE_FORMAT);

  depthOnlyShader.programName("indirectDepthOnlyShader")
    .vertexPath(EVERYTHING_SHADERS_ROOT "indirect_shadow_map.vert.spv")
    .fragmentPath(EVERYTHING_SHADERS_ROOT "indirect_depth.frag.spv")
    .vertexFormat(sceneManager.getCompressedVertexFormatDescription())
    .depthOutputFormat(DeferredTextureBunch::DEPTH_FORMAT)
    .init();

  textures.reserve(sceneManager.getImages().size());
  for (const auto& img : sceneManager.getImages())
  {
    textures.emplace_back();
    auto& tex = textures.back();
    tex.name("albedo")
      .size(img.width, img.height)
      .data(&img.data.front())
      .format(vk::Format::eR8G8B8A8Srgb)
      .useSampled()
      .init(&app.getCmdBuf());
    shader.addPersistentBinding(tex, app.getDefaultSampler());
  }

  shader.init();
}

struct Matrices
{
  glm::mat4 projView;
  glm::mat4 view;
};

void BindlessScene::render(spg::App& app, DeferredTextureBunch& target)
{
  Matrices matrices;
  matrices.projView = app.getWorldViewProj();
  matrices.view = app.getWorldView();

  shader.dispatch(app.getCmdBuf())
    .geometry(sceneManager.getVertexBuffer(), sceneManager.getIndexBuffer())
    .indirect(indirect, indirectCount)
    .bind(0, instanceInfo)
    .attachAsDepth(target.depth)
    .attach(target.albedo)
    .attach(target.normalEmissive)
    .pushVertex(matrices);
}

void BindlessScene::renderShadowMap(spg::App& app, ShadowMap& target)
{
  for (unsigned idx = 0; idx < ShadowMap::NUM_LEVELS; ++idx)
  {
    Matrices matrices;
    Camera& cam = target.getCamera(idx);
    matrices.projView = cam.projTm(1.0f) * cam.viewTm();
    matrices.view = cam.viewTm();

    depthOnlyShader.dispatch(app.getCmdBuf())
      .geometry(sceneManager.getVertexBuffer(), sceneManager.getIndexBuffer())
      .indirect(indirect, indirectCount)
      .bind(0, instanceInfo)
      .attachAsDepth(target.getTexture(idx))
      .pushVertex(matrices);
  }
}
