
add_library(spaghetti_renderer
        spaghetti.cpp
        Renderer.cpp
        WorldRenderer.cpp
        primitives/Texture.cpp
        primitives/Buffer.cpp
        primitives/FragmentOnlyShader.cpp
        primitives/VertexFragmentShader.cpp
)

target_include_directories(spaghetti_renderer PUBLIC ..)

target_link_libraries(spaghetti_renderer PRIVATE tinygltf glfw etna glm wsi gui scene render_utils)

target_add_shaders(spaghetti_renderer
        shaders/static_mesh.frag
        shaders/static_mesh.vert
        shaders/fullscreen.vert
        shaders/deferred_point_lights.frag
        shaders/decals.frag
        shaders/cluster_assign.comp
)

target_include_directories(spaghetti_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)
