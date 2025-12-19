#include "SwapManager.h"
#include "../PTE/PageTableEntry.h"

#include <fstream>

SwapManager::SwapManager(VirtualMemory& virtualMemory, const std::filesystem::path& path)
	: m_virtualMemory(virtualMemory)
	, m_swapFilePath(path)
{
}

uint32_t SwapManager::AllocateSlot(const uint32_t virtualPageNumber)
{
	const auto slot = m_nextSlot++;
	m_pageNumberToSlotMap[virtualPageNumber] = slot;
	m_slotToPageNumberMap[slot] = virtualPageNumber;
	return slot;
}

void SwapManager::FreeSlot(const uint32_t virtualPageNumber)
{
	const auto it = m_pageNumberToSlotMap.find(virtualPageNumber);

	if (it == m_pageNumberToSlotMap.end())
	{
		return;
	}
	const auto slot = it->second;
	m_slotToPageNumberMap.erase(slot);
	m_pageNumberToSlotMap.erase(virtualPageNumber);
}

void SwapManager::ReadPage(const uint32_t slot, const uint32_t virtualPageNumber) const
{
	std::ifstream file(m_swapFilePath, std::ios::binary);
	if (!file.is_open())
	{
		return;
	}

	file.seekg(slot * PTE::PAGE_SIZE);
	std::vector<uint8_t> buffer(PTE::PAGE_SIZE);
	file.read(reinterpret_cast<char*>(buffer.data()), PTE::PAGE_SIZE);

	const uint32_t physicalAddress = virtualPageNumber * PTE::PAGE_SIZE;
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		m_virtualMemory.Write8(physicalAddress + i, buffer[i], Privilege::Supervisor);
	}
}

void SwapManager::WritePage(const uint32_t slot, const uint32_t virtualPageNumber) const
{
	std::fstream file(m_swapFilePath, std::ios::binary);

	if (!file.is_open())
	{
		return;
	}

	std::vector<uint8_t> buffer(PTE::PAGE_SIZE);
	const uint32_t physicalAddress = virtualPageNumber * PTE::PAGE_SIZE;
	for (size_t i = 0; i < buffer.size(); ++i)
	{
		buffer[i] = m_virtualMemory.Read8(physicalAddress + i, Privilege::Supervisor, false);
	}
	file.seekp(slot * PTE::PAGE_SIZE);
	file.write(reinterpret_cast<const char*>(buffer.data()), PTE::PAGE_SIZE);
}

bool SwapManager::HasSlot(const uint32_t virtualPageNumber) const
{
	return m_pageNumberToSlotMap.contains(virtualPageNumber);
}

uint32_t SwapManager::GetSlot(const uint32_t virtualPageNumber) const
{
	const auto it = m_pageNumberToSlotMap.find(virtualPageNumber);
	return it != m_pageNumberToSlotMap.end()
		? it->second
		: UINT32_MAX;
}
