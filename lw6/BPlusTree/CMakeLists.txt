find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
add_executable(BPlusTree BPlusTree.cpp main.cpp
        ../../lw8/Calculator/main.cpp)
add_executable(TestBPlusTree BPlusTree.cpp TreeTest.cpp)

target_link_libraries(TestBPlusTree PRIVATE Catch2::Catch2WithMain)

target_include_directories(BPlusTree PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(BPlusTree PRIVATE BOOST_ALL_NO_LIB)