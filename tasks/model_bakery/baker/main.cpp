#include "Model.h"


#include <iostream>
#include <filesystem>
#include <fstream>

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Not enough command line arguments, expected a model path." << std::endl;
    return EXIT_FAILURE;
  }

  std::filesystem::path modelPath(argv[1]);
  std::cout << "Loading model " << modelPath.string() << std::endl;

  auto maybeModel = Model::fromGltf(modelPath.string());
  if (!maybeModel.has_value())
  {
    std::cerr << "Failed to load model " << modelPath.string() << std::endl;
    return EXIT_FAILURE;
  }

  Model& model = *maybeModel;

  std::filesystem::path outPath = modelPath;
  outPath.replace_extension(outPath.filename().string() + "_baked");

  outPath.replace_extension("bin");
  model.toBin(outPath.string());

  outPath.replace_extension("gltf");
  model.toGltf(outPath.string());

  return EXIT_SUCCESS;
}
