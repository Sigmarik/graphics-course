
add_executable(model_bakery_baker
  main.cpp Model.cpp
)

target_link_libraries(model_bakery_baker
  PRIVATE glm tinygltf)
