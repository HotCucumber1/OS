file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/input/
)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

find_package(Boost REQUIRED COMPONENTS thread)

add_executable(MtSearch MtSearch.cpp main.cpp)
add_executable(SearchBenchmark MtSearch.cpp SearchBenchmark.cpp)

target_include_directories(MtSearch PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MtSearch PRIVATE Boost::thread Threads::Threads m)
target_link_libraries(SearchBenchmark PRIVATE Catch2::Catch2WithMain)