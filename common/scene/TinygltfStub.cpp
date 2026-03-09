#include <tiny_gltf.h>

namespace tinygltf {

bool LoadImageData(Image* /*image*/, const int /*image_idx*/, std::string* /*err*/,
                   std::string* /*warn*/, int /*req_width*/, int /*req_height*/,
                   const unsigned char* /*bytes*/, int /*size*/, void* /*user_data*/) {
  // This stub is used when tinygltf is compiled with TINYGLTF_NO_STB_IMAGE.
  // It returns false to indicate that no image was loaded, which is fine
  // because we do not use embedded images.
  return true;
}

} // namespace tinygltf
