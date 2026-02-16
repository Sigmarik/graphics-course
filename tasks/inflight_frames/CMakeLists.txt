add_executable(inflight_frames
  main.cpp
  App.cpp
)

target_link_libraries(inflight_frames
  PRIVATE glfw etna glm wsi gui)

target_add_shaders(inflight_frames
        shaders/toy.vert
        shaders/toy.frag
        shaders/intermediate.frag
)

target_include_directories(inflight_frames PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_compile_definitions(inflight_frames PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")
