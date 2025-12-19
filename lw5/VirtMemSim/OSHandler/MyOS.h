#pragma once
#include "../SwapManager/SwapManager.h"
#include "DisplaceAlgorithm/DisplaceAlgorithmInterface.h"
#include "IOSHandler.h"
#include "ReferenceCollector.h"

#include <filesystem>

class MyOS : public IOSHandler
{
public:
	explicit MyOS(std::filesystem::path swapFilePath, PhysicalMemory& physicalMem);

	bool OnPageFault(
		VirtualMemory& vm,
		uint32_t virtualPageNumber,
		Access access,
		PageFaultReason reason) override;

	size_t GetPageFaultCount() const;

	size_t GetSwapWriteCount() const;

	size_t GetSwapReadCount() const;

	void TimerInterrupt();

private:
	bool HandlePageFault(VirtualMemory& vm, uint32_t virtualPageNumber, Access access, PageFaultReason reason);

	void AddPageAccess(uint32_t pageNumber) const;

	bool HandleNotPresentFault(VirtualMemory& vm, uint32_t virtualPageNumber, Access access);

	uint32_t FindFreeFrame() const;

	void DisplacePage(VirtualMemory& vm, uint32_t frameNumber);

	void InitSwap(VirtualMemory& vm);

	bool EnsurePageTableExists(const VirtualMemory& vm, uint32_t virtualPageNumber) const;

	std::filesystem::path m_swapFilePath;
	std::unique_ptr<DisplaceAlgorithmInterface> m_displaceAlgorithm;
	std::unique_ptr<ReferenceCollector> m_references;
	std::unique_ptr<SwapManager> m_swapManager = nullptr;
	PhysicalMemory& m_physicalMem;

	size_t m_pageFaultCount = 0; // TODO надо ли?
	size_t m_swapWriteCount = 0;
	size_t m_swapReadCount = 0;
	size_t m_dirtyEvictCount = 0;
	size_t m_framesNumber = 0;
	int m_agingTimer = 0;

	bool m_inPageFaultHandler = false;
};
