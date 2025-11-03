
add_executable(many_objects
        main.cpp
        App.cpp
        Renderer.cpp
        WorldRenderer.cpp
)

target_link_libraries(many_objects
  PRIVATE glfw etna glm wsi gui scene render_utils)

target_add_shaders(many_objects
        shaders/static_mesh.frag
        shaders/static_mesh.vert
)
