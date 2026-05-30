add_executable(bindless
        main.cpp
        Scene.cpp
)

target_include_directories(bindless PUBLIC ..)

target_link_libraries(bindless
        PRIVATE etna wsi gui tinygltf spaghetti_renderer)

target_add_shaders(bindless
        shaders/indirect.frag
        shaders/indirect.vert
)

add_compile_definitions(bindless PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")

target_include_directories(bindless PRIVATE ${PROJECT_SOURCE_DIR}/include)
