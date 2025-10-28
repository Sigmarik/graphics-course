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

  std::filesystem::path outPath =
    modelPath.replace_filename(modelPath.filename().stem().string() + "_baked");

  model.toBin(outPath.replace_extension("bin").string());
  model.toGltf(outPath.replace_extension("gltf").string());

  return EXIT_SUCCESS;
}
