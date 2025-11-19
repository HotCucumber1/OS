file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/input/
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/input/
)
find_package(ZLIB REQUIRED)

add_executable(ExePacker DataPacker.cpp main.cpp
        ../../lw5/VirtMemSim/Physical/PhysicalMemory.h
        ../../lw5/VirtMemSim/Physical/PhysicalMemory.cpp
        ../../lw5/VirtMemSim/Physical/PhysicalMemory.h
        ../../lw5/VirtMemSim/PTE/PageTableEntry.h
        ../../lw5/VirtMemSim/Virtual/VirtualMemory.cpp
        ../../lw5/VirtMemSim/Virtual/VirtualMemory.h
        ../../lw5/VirtMemSim/OSHandler/OSHandler.cpp
        ../../lw5/VirtMemSim/OSHandler/IOSHandler.h)

target_link_libraries(ExePacker ZLIB::ZLIB)