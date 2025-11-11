#include "Model.h"

#include "tiny_gltf.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stack>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#pragma pack(push, 1)
struct CompressedVertex
{
  float posX = 0, posY = 0, posZ = 0;
  int8_t normX = 0, normY = 127, normZ = 0;
  unsigned char _padding_0[1]{};
  float uvX = 0, uvY = 0;
  int8_t tanX = 127, tanY = 0, tanZ = 0, tanW = 127;
  unsigned char _padding_1[4]{};
};
#pragma pack(pop)

struct Vertex
{
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 texCoords{};
  glm::vec3 tangent{};
};

std::optional<tinygltf::Model> load_model(const std::string& path)
{
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;

  std::filesystem::path scenePath(path);

  if (scenePath.extension() != ".gltf")
  {
    std::cerr << "Unknown file extension " << scenePath.extension() << std::endl;
    return std::nullopt;
  }

  std::string error;
  std::string warning;
  bool success = loader.LoadASCIIFromFile(&model, &error, &warning, scenePath.string());

  if (!success)
  {
    std::cerr << "Failed to load glTF file " << path << std::endl;
    if (!error.empty())
    {
      std::cerr << error << std::endl;
    }
    return std::nullopt;
  }

  if (!warning.empty())
  {
    std::cerr << warning << std::endl;
  }

  return model;
}

static void read_primitive(
  const tinygltf::Primitive& primitive,
  const tinygltf::Model& model,
  std::vector<Vertex>& vertices,
  std::vector<uint32_t>& indices);

static CompressedVertex compress_vertex(const Vertex& vertex);

template <class T>
static void append_as_bytes(std::vector<unsigned char>& bytes, const std::vector<T>& data)
{
  bytes.reserve(bytes.size() + data.size() * sizeof(T));

  for (const T& item : data)
  {
    const unsigned char* rawBytes = reinterpret_cast<const unsigned char*>(&item);
    bytes.insert(bytes.end(), rawBytes, rawBytes + sizeof(T));
  }
}

void optimize_model(tinygltf::Model& model)
{
  std::vector<CompressedVertex> globalVertices;
  std::vector<uint32_t> globalIndices;

  std::vector<tinygltf::Accessor> globalAccessors;

  for (auto& mesh : model.meshes)
  {
    for (auto& prim : mesh.primitives)
    {
      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;

      read_primitive(prim, model, vertices, indices);

      size_t vertexCount = vertices.size();
      size_t indexCount = indices.size();

      prim.indices = static_cast<int>(globalAccessors.size());
      globalAccessors.emplace_back();
      auto& indexAccessor = globalAccessors.back();
      indexAccessor.bufferView = 0;
      indexAccessor.byteOffset = globalIndices.size() * sizeof(uint32_t);
      indexAccessor.count = indexCount;
      indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
      indexAccessor.type = TINYGLTF_TYPE_SCALAR;
      indexAccessor.minValues.emplace_back(0);
      uint32_t maxIndex = 0;
      for (uint32_t idx : indices)
      {
        maxIndex = std::max(idx, maxIndex);
      }
      indexAccessor.maxValues.emplace_back(maxIndex);

      prim.attributes["POSITION"] = static_cast<int>(globalAccessors.size());
      globalAccessors.emplace_back();
      auto& positionAccessor = globalAccessors.back();
      positionAccessor.bufferView = 1;
      positionAccessor.byteOffset = globalVertices.size() * sizeof(CompressedVertex) + 0;
      positionAccessor.count = vertexCount;
      positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      positionAccessor.type = TINYGLTF_TYPE_VEC3;

      glm::vec3 minPos = vertices[0].position;
      glm::vec3 maxPos = vertices[0].position;
      for (const auto& vtx : vertices)
      {
        minPos.x = std::min(minPos.x, vtx.position.x);
        minPos.y = std::min(minPos.y, vtx.position.y);
        minPos.z = std::min(minPos.z, vtx.position.z);
        maxPos.x = std::max(maxPos.x, vtx.position.x);
        maxPos.y = std::max(maxPos.y, vtx.position.y);
        maxPos.z = std::max(maxPos.z, vtx.position.z);
      }
      positionAccessor.minValues.push_back(minPos.x);
      positionAccessor.minValues.push_back(minPos.y);
      positionAccessor.minValues.push_back(minPos.z);
      positionAccessor.maxValues.push_back(maxPos.x);
      positionAccessor.maxValues.push_back(maxPos.y);
      positionAccessor.maxValues.push_back(maxPos.z);

      prim.attributes["NORMAL"] = static_cast<int>(globalAccessors.size());
      globalAccessors.emplace_back();
      auto& normalAccessor = globalAccessors.back();
      normalAccessor.bufferView = 1;
      normalAccessor.byteOffset = globalVertices.size() * sizeof(CompressedVertex) + 12;
      normalAccessor.count = vertexCount;
      normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_BYTE;
      normalAccessor.type = TINYGLTF_TYPE_VEC3;
      normalAccessor.normalized = true;

      prim.attributes["TEXCOORD_0"] = static_cast<int>(globalAccessors.size());
      globalAccessors.emplace_back();
      auto& uvAccessor = globalAccessors.back();
      uvAccessor.bufferView = 1;
      uvAccessor.byteOffset = globalVertices.size() * sizeof(CompressedVertex) + 16;
      uvAccessor.count = vertexCount;
      uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
      uvAccessor.type = TINYGLTF_TYPE_VEC2;

      prim.attributes["TANGENT"] = static_cast<int>(globalAccessors.size());
      globalAccessors.emplace_back();
      auto& tangentAccessor = globalAccessors.back();
      tangentAccessor.bufferView = 1;
      tangentAccessor.byteOffset = globalVertices.size() * sizeof(CompressedVertex) + 24;
      tangentAccessor.count = vertexCount;
      tangentAccessor.componentType = TINYGLTF_COMPONENT_TYPE_BYTE;
      tangentAccessor.type = TINYGLTF_TYPE_VEC4;
      tangentAccessor.normalized = true;

      for (const Vertex& vtx : vertices)
      {
        globalVertices.push_back(compress_vertex(vtx));
      }
      globalIndices.insert(globalIndices.end(), indices.begin(), indices.end());
    }
  }

  tinygltf::BufferView indexView;
  indexView.buffer = 0;
  indexView.byteOffset = 0;
  indexView.byteLength = globalIndices.size() * sizeof(uint32_t);

  tinygltf::BufferView vertexView;
  vertexView.buffer = 0;
  vertexView.byteOffset = globalIndices.size() * sizeof(uint32_t);
  vertexView.byteLength = globalVertices.size() * sizeof(CompressedVertex);
  vertexView.byteStride = sizeof(CompressedVertex);

  tinygltf::Buffer buffer;
  append_as_bytes(buffer.data, globalIndices);
  append_as_bytes(buffer.data, globalVertices);

  model.accessors = globalAccessors;
  model.buffers = {buffer};
  model.bufferViews = {indexView, vertexView};

  model.extensionsUsed.push_back("KHR_mesh_quantization");
  model.extensionsRequired.push_back("KHR_mesh_quantization");
}

void export_model(const tinygltf::Model& model, const std::string& path)
{
  tinygltf::TinyGLTF loader;
  loader.WriteGltfSceneToFile(&model, path, false, false, true, false);
}

static int8_t compress_float(float value)
{
  assert(value >= -1.0);
  assert(value <= 1.0);

  return static_cast<int8_t>(value * 127.0);
}

void read_primitive(
  const tinygltf::Primitive& prim,
  const tinygltf::Model& model,
  std::vector<Vertex>& vertices,
  std::vector<uint32_t>& indices)
{
  if (prim.mode != TINYGLTF_MODE_TRIANGLES)
  {
    std::cerr << "Encountered a non-triangles primitive, these are not supported for now, "
                 "skipping it!"
              << std::endl;
    return;
  }

  const auto normalIt = prim.attributes.find("NORMAL");
  const auto tangentIt = prim.attributes.find("TANGENT");
  const auto texcoordIt = prim.attributes.find("TEXCOORD_0");

  const bool hasNormals = normalIt != prim.attributes.end();
  const bool hasTangents = tangentIt != prim.attributes.end();
  const bool hasTexcoord = texcoordIt != prim.attributes.end();

  std::array accessorIndices{
    prim.indices,
    prim.attributes.at("POSITION"),
    hasNormals ? normalIt->second : -1,
    hasTangents ? tangentIt->second : -1,
    hasTexcoord ? texcoordIt->second : -1,
  };

  std::array accessors{
    &model.accessors[prim.indices],
    &model.accessors[accessorIndices[1]],
    hasNormals ? &model.accessors[accessorIndices[2]] : nullptr,
    hasTangents ? &model.accessors[accessorIndices[3]] : nullptr,
    hasTexcoord ? &model.accessors[accessorIndices[4]] : nullptr,
  };

  std::array bufViews{
    &model.bufferViews[accessors[0]->bufferView],
    &model.bufferViews[accessors[1]->bufferView],
    hasNormals ? &model.bufferViews[accessors[2]->bufferView] : nullptr,
    hasTangents ? &model.bufferViews[accessors[3]->bufferView] : nullptr,
    hasTexcoord ? &model.bufferViews[accessors[4]->bufferView] : nullptr,
  };

  const std::size_t vertexCount = accessors[1]->count;

  std::array ptrs{
    reinterpret_cast<const std::byte*>(model.buffers[bufViews[0]->buffer].data.data()) +
      bufViews[0]->byteOffset + accessors[0]->byteOffset,
    reinterpret_cast<const std::byte*>(model.buffers[bufViews[1]->buffer].data.data()) +
      bufViews[1]->byteOffset + accessors[1]->byteOffset,
    hasNormals
      ? reinterpret_cast<const std::byte*>(model.buffers[bufViews[2]->buffer].data.data()) +
        bufViews[2]->byteOffset + accessors[2]->byteOffset
      : nullptr,
    hasTangents
      ? reinterpret_cast<const std::byte*>(model.buffers[bufViews[3]->buffer].data.data()) +
        bufViews[3]->byteOffset + accessors[3]->byteOffset
      : nullptr,
    hasTexcoord
      ? reinterpret_cast<const std::byte*>(model.buffers[bufViews[4]->buffer].data.data()) +
        bufViews[4]->byteOffset + accessors[4]->byteOffset
      : nullptr,
  };

  std::array strides{
    bufViews[0]->byteStride != 0 ? bufViews[0]->byteStride
                                 : tinygltf::GetComponentSizeInBytes(accessors[0]->componentType) *
        tinygltf::GetNumComponentsInType(accessors[0]->type),
    bufViews[1]->byteStride != 0 ? bufViews[1]->byteStride
                                 : tinygltf::GetComponentSizeInBytes(accessors[1]->componentType) *
        tinygltf::GetNumComponentsInType(accessors[1]->type),
    hasNormals ? (bufViews[2]->byteStride != 0
                    ? bufViews[2]->byteStride
                    : tinygltf::GetComponentSizeInBytes(accessors[2]->componentType) *
                      tinygltf::GetNumComponentsInType(accessors[2]->type))
               : 0,
    hasTangents ? (bufViews[3]->byteStride != 0
                     ? bufViews[3]->byteStride
                     : tinygltf::GetComponentSizeInBytes(accessors[3]->componentType) *
                       tinygltf::GetNumComponentsInType(accessors[3]->type))
                : 0,
    hasTexcoord ? (bufViews[4]->byteStride != 0
                     ? bufViews[4]->byteStride
                     : tinygltf::GetComponentSizeInBytes(accessors[4]->componentType) *
                       tinygltf::GetNumComponentsInType(accessors[4]->type))
                : 0,
  };

  for (std::size_t i = 0; i < vertexCount; ++i)
  {
    auto& vtx = vertices.emplace_back();
    glm::vec3 pos;
    // Fall back to 0 in case we don't have something.
    // NOTE: if tangents are not available, one could use http://mikktspace.com/
    // NOTE: if normals are not available, reconstructing them is possible but will look ugly
    glm::vec3 normal{0.0, 1.0, 0.0};
    glm::vec3 tangent{1.0, 0.0, 0.0};
    glm::vec2 texcoord{0};
    std::memcpy(&pos, ptrs[1], sizeof(pos));

    // NOTE: it's faster to do a template here with specializations for all combinations than to
    // do ifs at runtime. Also, SIMD should be used. Try implementing this!
    if (hasNormals)
      std::memcpy(&normal, ptrs[2], sizeof(normal));
    if (hasTangents)
      std::memcpy(&tangent, ptrs[3], sizeof(tangent));
    if (hasTexcoord)
      std::memcpy(&texcoord, ptrs[4], sizeof(texcoord));


    vtx.position = pos;
    vtx.normal = normal;
    vtx.texCoords = texcoord;
    vtx.tangent = tangent;

    ptrs[1] += strides[1];
    if (hasNormals)
      ptrs[2] += strides[2];
    if (hasTangents)
      ptrs[3] += strides[3];
    if (hasTexcoord)
      ptrs[4] += strides[4];
  }

  // Indices are guaranteed to have no stride
  assert(bufViews[0]->byteStride == 0);
  const std::size_t indexCount = accessors[0]->count;
  if (accessors[0]->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
  {
    for (std::size_t i = 0; i < indexCount; ++i)
    {
      std::uint16_t index;
      std::memcpy(&index, ptrs[0], sizeof(index));
      indices.push_back(index);
      ptrs[0] += 2;
    }
  }
  else if (accessors[0]->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
  {
    const std::size_t lastTotalIndices = indices.size();
    indices.resize(lastTotalIndices + indexCount);
    std::memcpy(indices.data() + lastTotalIndices, ptrs[0], sizeof(indices[0]) * indexCount);
  }
}

CompressedVertex compress_vertex(const Vertex& vertex)
{
  CompressedVertex comp;
  comp.posX = vertex.position.x;
  comp.posY = vertex.position.y;
  comp.posZ = vertex.position.z;
  comp.normX = compress_float(vertex.normal.x);
  comp.normY = compress_float(vertex.normal.y);
  comp.normZ = compress_float(vertex.normal.z);
  comp.uvX = vertex.texCoords.x;
  comp.uvY = vertex.texCoords.y;
  comp.tanX = compress_float(vertex.tangent.x);
  comp.tanY = compress_float(vertex.tangent.y);
  comp.tanZ = compress_float(vertex.tangent.z);

  return comp;
}
