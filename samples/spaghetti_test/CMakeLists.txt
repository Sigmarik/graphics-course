add_executable(spaghetti_test
        main.cpp
        Scene.cpp
)

target_link_libraries(spaghetti_test PRIVATE glm etna spaghetti_renderer)

#target_add_shaders(spaghetti_test shaders/simple.comp)