include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(
        VirtMemSim
        OSHandler/OSHandler.cpp
        Physical/PhysicalMemory.cpp
        Virtual/VirtualMemory.cpp
)

add_executable(
        TestVirtualMem
        OSHandler/OSHandler.cpp
        Physical/PhysicalMemory.cpp
        Virtual/VirtualMemory.cpp
        test/VirtualMemoryTest.cpp
)
add_executable(
        TestPhysicalMem
        OSHandler/OSHandler.cpp
        Physical/PhysicalMemory.cpp
        test/PhysicalMemoryTest.cpp
)

target_link_libraries(TestVirtualMem PRIVATE Catch2::Catch2WithMain)
target_link_libraries(TestPhysicalMem PRIVATE Catch2::Catch2WithMain)