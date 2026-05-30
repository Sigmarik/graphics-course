
add_executable(local_shadertoy2
  main.cpp
  App.cpp
)

target_link_libraries(local_shadertoy2
  PRIVATE glfw etna glm wsi gui)

target_add_shaders(local_shadertoy2
        shaders/toy.vert
        shaders/toy.frag
        shaders/intermediate.frag
)

target_include_directories(local_shadertoy2 PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_compile_definitions(local_shadertoy2 PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")
