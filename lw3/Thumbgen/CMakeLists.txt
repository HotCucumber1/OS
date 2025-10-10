file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/input/
)

file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/garbage/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/garbage/
)

include(FetchContent)
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)
include_directories(${stb_SOURCE_DIR})

FetchContent_Declare(
        cxxopts
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
        GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(cxxopts)
include_directories(${cxxopts_SOURCE_DIR})

find_package(Boost REQUIRED COMPONENTS thread)

add_executable(Thumbgen Threads/Threads.cpp main.cpp)

target_include_directories(Thumbgen PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(Thumbgen PRIVATE cxxopts)
target_link_libraries(Thumbgen PRIVATE Boost::thread Threads::Threads m)

target_compile_definitions(Thumbgen PRIVATE STB_IMAGE_IMPLEMENTATION)