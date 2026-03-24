add_executable(everything
        main.cpp
        Scene.cpp
        DeferredTextureBunch.cpp
        BindlessScene.cpp
)

target_include_directories(everything PUBLIC ..)

target_link_libraries(everything
        PRIVATE etna wsi gui tinygltf spaghetti_renderer)

target_add_shaders(everything
        shaders/combine_lighting.frag
        shaders/indirect.frag
        shaders/indirect.vert
)

add_compile_definitions(everything PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")

target_include_directories(everything PRIVATE ${PROJECT_SOURCE_DIR}/include)
