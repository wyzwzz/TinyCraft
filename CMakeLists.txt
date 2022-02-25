cmake_minimum_required(VERSION 3.16)

project(TinyCraft LANGUAGES CXX)

include(deps/glm.cmake)

file(
    GLOB
    TinyCraft_SRCS
    "src/*.hpp"
    "src/*.cpp"
)

add_executable(TinyCraft ${TinyCraft_SRCS})


target_link_libraries(
    TinyCraft PRIVATE
    glm::glm
    
)