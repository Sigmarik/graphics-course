#include "Scene.h"

struct GrassVertex
{
  glm::vec3 pos;
  float relativeHeight;
};

void Scene::initialize()
{
  etna::VertexByteStreamFormatDescription grassVtxFormat = {
    .stride = sizeof(GrassVertex),
    .attributes = {
      etna::VertexByteStreamFormatDescription::Attribute{
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = 0,
      },
      etna::VertexByteStreamFormatDescription::Attribute{
        .format = vk::Format::eR32Sfloat,
        .offset = sizeof(glm::vec3),
      },
    }
  };

  std::vector<GrassVertex> grassVertices
  {
    GrassVertex{
      .pos = glm::vec3(-0.05, 0, 0),
      .relativeHeight = 0,
    },
    GrassVertex{
      .pos = glm::vec3(0.05, 0, 0),
      .relativeHeight = 0,
    },
    GrassVertex{
      .pos = glm::vec3(0, 0.3, 0),
      .relativeHeight = 1,
    },
  };

  vertices.name("vertices")
    .useVertex()
    .initAndCopy(grassVertices);

  indices.name("indices")
    .useIndex()
    .initAndCopy(std::vector<uint32_t>{0, 1, 2});

  depth.name("depth")
    .format(vk::Format::eD32Sfloat)
    .useDepthStencil()
    .size(getResolution().x, getResolution().y)
    .init(&getCmdBuf());

  shader.programName("grassShader")
    .vertexPath(GRASS_SHADERS_ROOT "grass.vert.spv")
    .fragmentPath(GRASS_SHADERS_ROOT "grass.frag.spv")
    .vertexFormat(grassVtxFormat)
    .depthOutputFormat(depth.raw().getFormat())
    .addColorAttachment(vk::Format::eB8G8R8A8Unorm)
  .init();
}

void Scene::render()
{
  shader.dispatch(getCmdBuf())
    .geometry(vertices, indices)
    .geomMapping(3)
    .instanceCount(10000000)
    .attachAsDepth(depth)
    .pushVertex(getWorldViewProj())
    .attach(getScreenAttachment(), getResolution());
}
