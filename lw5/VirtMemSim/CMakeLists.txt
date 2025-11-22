include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(
        VirtMemSim
        OSHandler/MyOS.cpp
        Physical/PhysicalMemory.cpp
        Virtual/VirtualMemory.cpp
)

add_executable(
        TestVirtualMemMock
        OSHandler/MyOS.cpp
        SwapManager/SwapManager.cpp
        OSHandler/DisplaceAlgorithm/Aging.cpp
        Physical/PhysicalMemory.cpp
        Virtual/VirtualMemory.cpp
        test/MockVirtualMemoryTest.cpp
)
add_executable(
        TestPhysicalMem
        OSHandler/MyOS.cpp
        Physical/PhysicalMemory.cpp
        test/PhysicalMemoryTest.cpp
)

add_executable(
        TestVirtualMem
        OSHandler/DisplaceAlgorithm/Aging.cpp
        OSHandler/MyOS.cpp
        SwapManager/SwapManager.cpp
        Physical/PhysicalMemory.cpp
        Virtual/VirtualMemory.cpp
        test/VirtualMemoryTest.cpp
)


target_link_libraries(TestVirtualMemMock PRIVATE Catch2::Catch2WithMain)
target_link_libraries(TestVirtualMem PRIVATE Catch2::Catch2WithMain)
target_link_libraries(TestPhysicalMem PRIVATE Catch2::Catch2WithMain)