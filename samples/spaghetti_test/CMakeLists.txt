add_executable(spaghetti_test
        main.cpp
        Scene.cpp
)

target_include_directories(spaghetti_test PUBLIC ..)

target_link_libraries(spaghetti_test
        PRIVATE etna wsi gui tinygltf spaghetti_renderer)

target_add_shaders(spaghetti_test
        shaders/indirect.frag
        shaders/indirect.vert
)

add_compile_definitions(spaghetti_test PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")

target_include_directories(spaghetti_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
