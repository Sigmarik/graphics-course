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

  auto maybeModel = load_model(modelPath.string());
  if (!maybeModel.has_value())
  {
    std::cerr << "Failed to load model " << modelPath.string() << std::endl;
    return EXIT_FAILURE;
  }

  auto& model = *maybeModel;

  optimize_model(model);

  std::filesystem::path outPath =
    modelPath.replace_filename(modelPath.filename().stem().string() + "_baked.gltf");

  export_model(model, outPath.string());

  return EXIT_SUCCESS;
}
