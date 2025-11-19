#pragma once
#include "../Virtual//VirtualMemory.h"

#include <filesystem>

class SwapManager
{
public:
	explicit SwapManager(
		VirtualMemory& virtualMemory,
		const std::filesystem::path& path);

	uint32_t AllocateSlot(uint32_t virtualPageNumber);

	void FreeSlot(uint32_t virtualPageNumber);

	void ReadPage(uint32_t slot, uint32_t virtualPageNumber);

	void WritePage(uint32_t slot, uint32_t virtualPageNumber);
};
