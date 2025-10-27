#include "Model.h"

#include "tiny_gltf.h"

#include <filesystem>
#include <iostream>
#include <stack>

static std::vector<Model> process_models(tinygltf::Model& model)
{
  std::vector<Model> models;

  for (auto& mesh : model.meshes)
  {
    Model composite;
    for (const auto& prim : mesh.primitives)
    {
      Model result;

      if (prim.mode != TINYGLTF_MODE_TRIANGLES)
      {
        std::cerr << "Encountered a non-triangles primitive, these are not supported for now, "
                     "skipping it!"
                  << std::endl;
        continue;
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
        bufViews[0]->byteStride != 0
          ? bufViews[0]->byteStride
          : tinygltf::GetComponentSizeInBytes(accessors[0]->componentType) *
            tinygltf::GetNumComponentsInType(accessors[0]->type),
        bufViews[1]->byteStride != 0
          ? bufViews[1]->byteStride
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
        auto& vtx = result.vertices.emplace_back();
        glm::vec3 pos;
        // Fall back to 0 in case we don't have something.
        // NOTE: if tangents are not available, one could use http://mikktspace.com/
        // NOTE: if normals are not available, reconstructing them is possible but will look ugly
        glm::vec3 normal{0};
        glm::vec3 tangent{0};
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
          result.indices.push_back(index);
          ptrs[0] += 2;
        }
      }
      else if (accessors[0]->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
      {
        const std::size_t lastTotalIndices = result.indices.size();
        result.indices.resize(lastTotalIndices + indexCount);
        std::memcpy(
          result.indices.data() + lastTotalIndices,
          ptrs[0],
          sizeof(result.indices[0]) * indexCount);
      }

      // Normally we would preserve model separation in our binary standard, but hey,
      // who am I to do something like this for brownie points, especially as I have already done
      // something very similar in the past.
      composite.append(result);
    }

    models.emplace_back(std::move(composite));
  }

  return models;
}

struct Instance
{
  uint32_t mesh = 0;
  glm::mat4x4 transform = glm::identity<glm::mat4x4>();
};

static std::vector<Instance> process_instances(const tinygltf::Model& model)
{
  std::vector nodeTransforms(model.nodes.size(), glm::identity<glm::mat4x4>());

  for (std::size_t nodeIdx = 0; nodeIdx < model.nodes.size(); ++nodeIdx)
  {
    const auto& node = model.nodes[nodeIdx];
    auto& transform = nodeTransforms[nodeIdx];

    if (!node.matrix.empty())
    {
      for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
          transform[i][j] = static_cast<float>(node.matrix[4 * i + j]);
    }
    else
    {
      if (!node.scale.empty())
        transform = scale(
          transform,
          glm::vec3(
            static_cast<float>(node.scale[0]),
            static_cast<float>(node.scale[1]),
            static_cast<float>(node.scale[2])));

      if (!node.rotation.empty())
        transform *= mat4_cast(glm::quat(
          static_cast<float>(node.rotation[3]),
          static_cast<float>(node.rotation[0]),
          static_cast<float>(node.rotation[1]),
          static_cast<float>(node.rotation[2])));

      if (!node.translation.empty())
        transform = translate(
          transform,
          glm::vec3(
            static_cast<float>(node.translation[0]),
            static_cast<float>(node.translation[1]),
            static_cast<float>(node.translation[2])));
    }
  }

  std::stack<std::size_t> vertices;
  for (auto vert : model.scenes[model.defaultScene].nodes)
    vertices.push(vert);

  while (!vertices.empty())
  {
    auto vert = vertices.top();
    vertices.pop();

    for (auto child : model.nodes[vert].children)
    {
      nodeTransforms[child] = nodeTransforms[vert] * nodeTransforms[child];
      vertices.push(child);
    }
  }

  std::vector<Instance> result;

  // Don't overallocate matrices, they are pretty chonky.
  {
    std::size_t totalNodesWithMeshes = 0;
    for (std::size_t i = 0; i < model.nodes.size(); ++i)
      if (model.nodes[i].mesh >= 0)
        ++totalNodesWithMeshes;
    result.reserve(totalNodesWithMeshes);
  }

  for (std::size_t i = 0; i < model.nodes.size(); ++i)
    if (model.nodes[i].mesh >= 0)
    {
      Instance instance;
      instance.mesh = model.nodes[i].mesh;
      instance.transform = nodeTransforms[i];
      result.emplace_back(std::move(instance));
    }

  return result;
}

std::optional<Model> Model::fromGltf(const std::string& path)
{
  tinygltf::TinyGLTF loader;
  tinygltf::Model model;

  Model composite;

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

  auto instances = process_instances(model);
  auto meshes = process_models(model);

  for (const auto& instance : instances)
  {
    composite.append(meshes[instance.mesh], instance.transform);
  }

  return composite;
}

void Model::toBin(const std::string&)
{
  std::cout << "Found " << vertices.size() << " vertices" << std::endl;

  // TODO: Implement
  std::cerr << "Implement `toBin`, please" << std::endl;
}

void Model::toGltf(const std::string&)
{
  // TODO: Implement
  std::cerr << "Implement `toGltf`, please" << std::endl;
}

void Model::append(const Model& model)
{
  index_t indexDelta = static_cast<index_t>(vertices.size());
  size_t startingIndex = indices.size();

  vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
  indices.insert(indices.end(), model.indices.begin(), model.indices.end());

  for (size_t idx = startingIndex; idx < indices.size(); ++idx)
  {
    indices[idx] += indexDelta;
  }
}

static glm::vec3 apply_transform(const glm::mat4& transform, const glm::vec3& vector, float alpha)
{
  return transform * glm::vec4(vector, alpha);
}

void Model::append(const Model& model, const glm::mat4& matrix)
{
  index_t indexDelta = static_cast<index_t>(vertices.size());
  size_t startingIndex = indices.size();

  indices.insert(indices.end(), model.indices.begin(), model.indices.end());

  for (size_t idx = startingIndex; idx < indices.size(); ++idx)
  {
    indices[idx] += indexDelta;
  }

  for (const auto& vertex : model.vertices)
  {
    Vertex transformed = vertex;
    transformed.position = apply_transform(matrix, transformed.position, 1.0f);
    transformed.normal = apply_transform(matrix, transformed.normal, 0.0f);
    transformed.tangent = apply_transform(matrix, transformed.tangent, 0.0f);
    vertices.emplace_back(transformed);
  }
}
