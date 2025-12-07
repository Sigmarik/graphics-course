
add_executable(deferred
  main.cpp
  App.cpp
  Renderer.cpp
  WorldRenderer.cpp
)

target_link_libraries(deferred
  PRIVATE glfw etna glm wsi gui scene render_utils)

target_add_shaders(deferred
  shaders/static_mesh.frag
  shaders/static_mesh.vert
)
