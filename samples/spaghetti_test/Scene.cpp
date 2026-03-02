#include "Scene.h"

struct InstanceInfo
{
  glm::mat4 transform;
  uint32_t textureIdx;
  uint32_t _pad[3];
};

void Scene::initialize()
{
  sceneManager.selectCompressedScene(GRAPHICS_COURSE_RESOURCES_ROOT "/scenes/low_poly_dark_town/scene_baked.gltf");

  auto instanceMeshes = sceneManager.getInstanceMeshes();
  auto instanceMatrices = sceneManager.getInstanceMatrices();
  auto meshes = sceneManager.getMeshes();
  auto relems = sceneManager.getRenderElements();

  std::vector<vk::DrawIndexedIndirectCommand> drawCommands;
  std::vector<InstanceInfo> instances;

  // Man do I LOVE fucking with GLFT models! They have SUCH A NICE structure,
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

  depth.name("depth")
    .format(vk::Format::eD32Sfloat)
    .useDepthStencil()
    .size(getResolution().x, getResolution().y)
    .init(&getCmdBuf());

  shader.programName("indirectShader")
    .vertexPath(SPAGHETTI_TEST_SHADERS_ROOT "indirect.vert.spv")
    .fragmentPath(SPAGHETTI_TEST_SHADERS_ROOT "indirect.frag.spv")
    .vertexFormat(sceneManager.getCompressedVertexFormatDescription())
    .depthOutputFormat(depth.raw().getFormat())
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm);

  textures.reserve(sceneManager.getImages().size());
  for (const auto& img : sceneManager.getImages())
  {
    if (std::holds_alternative<std::filesystem::path>(img))
    {
      const std::filesystem::path& path = std::get<std::filesystem::path>(img);
      textures.emplace_back();
      auto& tex = textures.back();
      tex.name("albedo").file(path.string()).init(&getCmdBuf());
      shader.addPersistentBinding(tex, getDefaultSampler());
    }
  }

  shader.init();
}

void Scene::render()
{
  shader.dispatch(getCmdBuf())
    .geometry(sceneManager.getVertexBuffer(), sceneManager.getIndexBuffer())
    .indirect(indirect, indirectCount)
    .bind(0, instanceInfo)
    .attachAsDepth(depth)
    .pushVertex(getWorldViewProj())
    .attach(getScreenAttachment(), getResolution());
}
