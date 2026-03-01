add_executable(spaghetti_test
        main.cpp
        Scene.cpp
)

target_link_libraries(spaghetti_test PRIVATE glm etna spaghetti_renderer)

target_add_shaders(spaghetti_test
        shaders/full_red.frag
        shaders/toy.frag
        shaders/intermediate.frag
)