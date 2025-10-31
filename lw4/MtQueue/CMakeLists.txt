add_executable(MtQueue main.cpp)
add_executable(TestMtQueue MtQueueTest.cpp)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

target_link_libraries(TestMtQueue PRIVATE Catch2::Catch2WithMain)
