add_executable(grass
        main.cpp
        Scene.cpp
)

target_include_directories(grass PUBLIC ..)

target_link_libraries(grass
        PRIVATE spaghetti_renderer)

target_add_shaders(grass
        shaders/grass.frag
        shaders/grass.vert
)

add_compile_definitions(grass PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")

target_include_directories(grass PRIVATE ${PROJECT_SOURCE_DIR}/include)
