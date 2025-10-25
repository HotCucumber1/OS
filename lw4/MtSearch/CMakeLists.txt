file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/input/
)

find_package(Boost REQUIRED COMPONENTS thread)

add_executable(MtSearch Controller/Controller.cpp main.cpp)

target_include_directories(MtSearch PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MtSearch PRIVATE Boost::thread Threads::Threads m)