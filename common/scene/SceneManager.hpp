#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <variant>
#include <etna/Buffer.hpp>
#include <etna/BlockingTransferHelper.hpp>
#include <etna/VertexInput.hpp>


// A single render element (relem) corresponds to a single draw call
// of a certain pipeline with specific bindings (including material data)
struct RenderElement
{
  std::uint32_t vertexOffset;
  std::uint32_t indexOffset;
  std::uint32_t indexCount;
  std::uint32_t albedoTextureIndex;
  // Not implemented!
  // Material* material;
};

// A mesh is a collection of relems. A scene may have the same mesh
// located in several different places, so a scene consists of **instances**,
// not meshes.
struct Mesh
{
  std::uint32_t firstRelem;
  std::uint32_t relemCount;
};

class SceneManager
{
public:
  SceneManager();

  void selectScene(std::filesystem::path path);
  void selectCompressedScene(std::filesystem::path path);

  // Every instance is a mesh drawn with a certain transform
  // NOTE: maybe you can pass some additional data through unused matrix entries?
  std::span<const glm::mat4x4> getInstanceMatrices() { return instanceMatrices; }
  std::span<const std::uint32_t> getInstanceMeshes() { return instanceMeshes; }

  // Every mesh is a collection of relems
  std::span<const Mesh> getMeshes() { return meshes; }

  // Every relem is a single draw call
  std::span<const RenderElement> getRenderElements() { return renderElements; }

  vk::Buffer getVertexBuffer() { return unifiedVbuf.get(); }
  vk::Buffer getIndexBuffer() { return unifiedIbuf.get(); }

  etna::VertexByteStreamFormatDescription getVertexFormatDescription();
  etna::VertexByteStreamFormatDescription getCompressedVertexFormatDescription();

private:
  std::optional<tinygltf::Model> loadModel(std::filesystem::path path);

  struct ProcessedInstances
  {
    std::vector<glm::mat4x4> matrices;
    std::vector<std::uint32_t> meshes;
  };

  ProcessedInstances processInstances(const tinygltf::Model& model) const;

  struct Vertex
  {
    // First 3 floats are position, 4th float is a packed normal
    glm::vec4 positionAndNormal;
    // First 2 floats are tex coords, 3rd is a packed tangent, 4th is padding
    glm::vec4 texCoordAndTangentAndPadding;
  };

  static_assert(sizeof(Vertex) == sizeof(float) * 8);

  struct ProcessedMeshes
  {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<RenderElement> relems;
    std::vector<Mesh> meshes;
  };

  struct ProcessedCompressedMeshes
  {
    const uint32_t* indexBuffer;
    size_t indexCount;
    const unsigned char* vertexBuffer;
    size_t vertexBufferSize;
    std::vector<RenderElement> relems;
    std::vector<Mesh> meshes;
  };
  ProcessedMeshes processMeshes(const tinygltf::Model& model) const;
  ProcessedCompressedMeshes processCompressedMeshes(const tinygltf::Model& model) const;
  void uploadData(std::span<const Vertex> vertices, std::span<const std::uint32_t>);
  void uploadCompressedData(
    const uint32_t* index_buffer,
    size_t index_count,
    const unsigned char* vertex_buffer,
    size_t vertex_buffer_size);

  void fillTextureInfo(const tinygltf::Model& model, const std::filesystem::path& root);

private:
  tinygltf::TinyGLTF loader;
  std::unique_ptr<etna::OneShotCmdMgr> oneShotCommands;
  etna::BlockingTransferHelper transferHelper;

  std::vector<RenderElement> renderElements;
  std::vector<Mesh> meshes;
  std::vector<glm::mat4x4> instanceMatrices;
  std::vector<std::uint32_t> instanceMeshes;

  struct ImageDescriptor
  {
    unsigned width = 0, height = 0;
    std::vector<unsigned char> data{};
  };
  std::vector<std::variant<std::filesystem::path, ImageDescriptor>> images;

  etna::Buffer unifiedVbuf;
  etna::Buffer unifiedIbuf;
};
