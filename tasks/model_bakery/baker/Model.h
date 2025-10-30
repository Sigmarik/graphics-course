#pragma once

#include <optional>
#include <string>

#include "tiny_gltf.h"

std::optional<tinygltf::Model> load_model(const std::string& path);

void optimize_model(tinygltf::Model& model);

void export_model(const tinygltf::Model& model, const std::string& path);
