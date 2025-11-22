cmake_minimum_required(VERSION 3.29)
project(OS)

set(CMAKE_CXX_STANDARD 20)
cmake_policy(SET CMP0167 NEW)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_subdirectory(lw1/task1)
add_subdirectory(lw1/task2)

add_subdirectory(lw2/ListProcLinux)
add_subdirectory(lw2/Zombie)
add_subdirectory(lw2/FlipCase)
add_subdirectory(lw2/ExePacker)

add_subdirectory(lw3/MtImgSim)
add_subdirectory(lw3/Thumbgen)

add_subdirectory(lw4/MtSearch)
add_subdirectory(lw4/MtQueue)

add_subdirectory(lw5/VirtMemSim)

add_subdirectory(lw6/BPlusTree)
