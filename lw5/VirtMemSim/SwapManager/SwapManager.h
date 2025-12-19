#pragma once
#include "../Virtual//VirtualMemory.h"

#include <filesystem>
#include <unordered_map>

class SwapManager
{
public:
	explicit SwapManager(
		VirtualMemory& virtualMemory,
		const std::filesystem::path& path);

	uint32_t AllocateSlot(uint32_t virtualPageNumber);

	void FreeSlot(uint32_t virtualPageNumber);

	void ReadPage(uint32_t slot, uint32_t virtualPageNumber) const;

	void WritePage(uint32_t slot, uint32_t virtualPageNumber) const;

	bool HasSlot(uint32_t virtualPageNumber) const;

	uint32_t GetSlot(uint32_t virtualPageNumber) const;

private:
	VirtualMemory& m_virtualMemory;
	std::filesystem::path m_swapFilePath;
	uint32_t m_nextSlot = 0;
	std::unordered_map<uint32_t, uint32_t> m_pageNumberToSlotMap;
	std::unordered_map<uint32_t, uint32_t> m_slotToPageNumberMap;
};
