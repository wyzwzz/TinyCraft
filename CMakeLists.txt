cmake_minimum_required(VERSION 3.16)

project(TinyCraft LANGUAGES CXX C)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include(deps/glm.cmake)
include(deps/glfw.cmake)

file(
    GLOB
    TinyCraft_SRCS
    "src/*.hpp"
    "src/*.cpp"
)
set(
    TinyCraft_SRCS
    ${TinyCraft_SRCS}
    deps/glad/src/glad.c
    deps/noise/noise.c
)
add_executable(TinyCraft ${TinyCraft_SRCS})


target_link_libraries(
    TinyCraft PRIVATE
    glm::glm
    glfw
)
target_include_directories(
    TinyCraft PRIVATE
    deps/glad/include
        deps/noise
        deps
)

target_compile_features(
    TinyCraft PRIVATE cxx_std_17
)