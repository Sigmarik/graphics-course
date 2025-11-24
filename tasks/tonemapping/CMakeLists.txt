
add_executable(tonemapping
  main.cpp
  App.cpp
)

target_link_libraries(tonemapping
  PRIVATE glfw etna glm wsi gui)

target_add_shaders(tonemapping
        shaders/toy.vert
        shaders/toy.frag
        shaders/tonemap.frag
        shaders/intermediate.frag
)

target_include_directories(tonemapping PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_compile_definitions(tonemapping PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")
