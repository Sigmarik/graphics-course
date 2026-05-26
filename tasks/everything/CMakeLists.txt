add_executable(everything
        main.cpp
        Scene.cpp
        DeferredTextureBunch.cpp
        BindlessScene.cpp
        ShadowMap.cpp
        SSAO.cpp
        Fog.cpp
        SSSS.cpp
        Particles.cpp
)

target_include_directories(everything PUBLIC ..)

target_link_libraries(everything
        PRIVATE etna wsi gui tinygltf spaghetti_renderer)

target_add_shaders(everything
        shaders/combine_lighting.frag
        shaders/ssao.frag
        shaders/fxaa.frag
        shaders/fog.frag
        shaders/direct_light.frag
        shaders/spatial_denoise.frag
        shaders/indirect.frag
        shaders/indirect_depth.frag
        shaders/indirect.vert
        shaders/indirect_shadow_map.vert
        shaders/subsurface.frag
        shaders/particles.vert
        shaders/particles.frag
        shaders/particle_iter.comp
        shaders/emitter_iter.comp
        shaders/particle_depth_sorter.comp
)

add_compile_definitions(everything PRIVATE
        TEXTURES_ROOT="${PROJECT_SOURCE_DIR}/resources/textures/")

target_include_directories(everything PRIVATE ${PROJECT_SOURCE_DIR}/include)
