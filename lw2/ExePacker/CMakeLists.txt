file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/input/
)
find_package(ZLIB REQUIRED)

add_executable(ExePacker DataPacker.cpp main.cpp)

target_link_libraries(ExePacker ZLIB::ZLIB)