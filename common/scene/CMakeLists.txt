
add_library(scene SceneManager.cpp TinygltfStub.cpp)

target_include_directories(scene PUBLIC ..)

target_link_libraries(scene PUBLIC glm tinygltf etna)
