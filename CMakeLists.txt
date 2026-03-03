cmake_minimum_required(VERSION 4.1)
project(VoxelFruitNinja)

set(CMAKE_CXX_STANDARD 20)


include(FetchContent)

# GLFW

FetchContent_Declare(
        glfw
        GIT_REPOSITORY  https://github.com/glfw/glfw.git
        GIT_TAG         3.4
        GIT_SHALLOW     TRUE
)
FetchContent_MakeAvailable(glfw)

# Glad
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad
        GIT_TAG        v2.0.6
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glad)

add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)

glad_add_library(glad
        REPRODUCIBLE
        LOADER
        API gl:core=4.6
)

# Executables
add_executable(VoxelFruitNinja main.cpp)

target_link_libraries(VoxelFruitNinja
        PRIVATE
        glfw
        glad
        opengl32
)