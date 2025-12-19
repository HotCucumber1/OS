#include "MyOS.h"

#include <utility>

#include "../SwapManager/SwapManager.h"
#include "../Virtual/VMContext.h"
#include "DisplaceAlgorithm/Aging.h"

constexpr int AGING_INTERVAL = 10;
constexpr int PAGE_DIR_SHIFT = 10;
constexpr int BLOCK_SIZE = 1024;

void InitializePageWithZeros(PhysicalMemory& pm, uint32_t physicalPageNumber);
PTE ReadPTEAtAddress(VirtualMemory& vm, uint32_t pteAddress);
uint32_t GetPTEAddress(const PhysicalMemory& pm, const VirtualMemory& vm, uint32_t virtualPageNumber);
void UpdatePTE(
	PhysicalMemory& pm,
	VirtualMemory& vm,
	uint32_t virtualPageNumber,
	uint32_t frameNumber,
	bool isWritable);

void KernelPanic(const std::string& message)
{
	throw std::runtime_error(message);
}

MyOS::MyOS(std::filesystem::path swapFilePath, PhysicalMemory& physicalMem)
	: m_swapFilePath(std::move(swapFilePath))
	, m_physicalMem(physicalMem)
{
	m_framesNumber = physicalMem.GetSize() / PTE::PAGE_SIZE;
	m_displaceAlgorithm = std::make_unique<Aging>(m_framesNumber);
}

bool MyOS::OnPageFault(
	VirtualMemory& vm,
	const uint32_t virtualPageNumber,
	const Access access,
	const PageFaultReason reason)
{
	InitSwap(vm);
	++m_pageFaultCount;

	if (m_inPageFaultHandler)
	{
		KernelPanic("Page fault in handler");
	}

	m_inPageFaultHandler = true;
	const auto res = HandlePageFault(vm, virtualPageNumber, access, reason);
	m_inPageFaultHandler = false;

	return res;
}

size_t MyOS::GetPageFaultCount() const
{
	return m_pageFaultCount;
}

size_t MyOS::GetSwapWriteCount() const
{
	return m_swapWriteCount;
}

size_t MyOS::GetSwapReadCount() const
{
	return m_swapReadCount;
}

void MyOS::TimerInterrupt()
{
	m_agingTimer++;
	if (m_agingTimer >= AGING_INTERVAL)
	{
		m_displaceAlgorithm->DoAge();
		m_agingTimer = 0;
	}
}

bool MyOS::HandlePageFault(
	VirtualMemory& vm,
	const uint32_t virtualPageNumber,
	const Access access,
	const PageFaultReason reason)
{
	switch (reason)
	{
	case PageFaultReason::NotPresent:
		return HandleNotPresentFault(vm, virtualPageNumber, access);
	default:
		return false;
	}
}

void MyOS::AddPageAccess(const uint32_t pageNumber) const
{
	m_references->AddReference(pageNumber);
}

bool MyOS::HandleNotPresentFault(
	VirtualMemory& vm,
	const uint32_t virtualPageNumber,
	const Access access)
{
	if (!EnsurePageTableExists(vm, virtualPageNumber))
	{
		return false;
	}

	auto freeFrame = FindFreeFrame();

	if (freeFrame == UINT32_MAX)
	{
		freeFrame = m_displaceAlgorithm->FindVictim();
		DisplacePage(vm, freeFrame);
	}

	if (m_swapManager->HasSlot(virtualPageNumber))
	{
		m_swapManager->ReadPage(m_swapManager->GetSlot(virtualPageNumber), freeFrame);
		++m_swapReadCount;
	}
	else
	{
		InitializePageWithZeros(m_physicalMem, freeFrame);
	}

	UpdatePTE(m_physicalMem, vm, virtualPageNumber, freeFrame, access == Access::Write);
	m_displaceAlgorithm->AccessPage(virtualPageNumber, access == Access::Write, freeFrame);

	return true;
}

bool MyOS::EnsurePageTableExists(const VirtualMemory& vm, const uint32_t virtualPageNumber) const
{
	const uint32_t pageDirAddress = vm.GetPageTableAddress();
	if (pageDirAddress == 0)
	{
		return false;
	}

	const uint32_t pageDirIndex = virtualPageNumber >> PAGE_DIR_SHIFT;
	const uint32_t pdeAddress = pageDirAddress + pageDirIndex * sizeof(PTE);

	PTE pde;
	pde.raw = m_physicalMem.Read32(pdeAddress);

	if (!pde.IsPresent())
	{
		const uint32_t newTableFrame = FindFreeFrame();
		if (newTableFrame == UINT32_MAX)
		{
			return false;
		}

		const uint32_t tableAddress = newTableFrame * PTE::PAGE_SIZE;
		PTE zeroPte;
		zeroPte.raw = 0;

		for (uint32_t i = 0; i < BLOCK_SIZE; ++i)
		{
			m_physicalMem.Write32(tableAddress + i * sizeof(PTE), zeroPte.raw);
		}

		PTE newPde;
		newPde.SetPresent(true);
		newPde.SetWritable(true);
		newPde.SetUser(true);
		newPde.SetFrame(newTableFrame);

		m_physicalMem.Write32(pdeAddress, newPde.raw);
	}

	return true;
}

uint32_t MyOS::FindFreeFrame() const
{
	for (uint32_t i = 0; i < m_framesNumber; ++i)
	{
		if (m_displaceAlgorithm->IsFrameOccupied(i))
		{
			continue;
		}
		return i;
	}
	return UINT32_MAX;
}

void MyOS::DisplacePage(VirtualMemory& vm, const uint32_t frameNumber)
{
	if (m_displaceAlgorithm->IsDirty(frameNumber))
	{
		++m_dirtyEvictCount;
		const auto slot = m_swapManager->AllocateSlot(frameNumber);
		m_swapManager->WritePage(slot, frameNumber);
		++m_swapWriteCount;
	}

	m_displaceAlgorithm->RemovePage(frameNumber);
}

void MyOS::InitSwap(VirtualMemory& vm)
{
	if (m_swapManager)
	{
		return;
	}
	m_swapManager = std::make_unique<SwapManager>(vm, m_swapFilePath);
}

void InitializePageWithZeros(PhysicalMemory& pm, const uint32_t physicalPageNumber)
{
	const auto physAddress = physicalPageNumber * PTE::PAGE_SIZE;
	for (uint32_t i = 0; i < PTE::PAGE_SIZE; ++i)
	{
		pm.Write8(physAddress + i, 0);
	}
}

void UpdatePTE(
	PhysicalMemory& pm,
	VirtualMemory& vm,
	const uint32_t virtualPageNumber,
	const uint32_t frameNumber,
	const bool isWritable)
{
	const uint32_t pteAddress = GetPTEAddress(pm, vm, virtualPageNumber);
	if (pteAddress == UINT32_MAX)
	{
		return;
	}

	PTE pte;
	pte.SetPresent(true);
	pte.SetWritable(isWritable);
	pte.SetUser(true);
	pte.SetAccessed(false);
	pte.SetDirty(false);
	pte.SetNX(false);
	pte.SetFrame(frameNumber);

	pm.Write32(pteAddress, pte.raw);
}

PTE ReadPTEAtAddress(VirtualMemory& vm, const uint32_t pteAddress)
{
	PTE pte;
	pte.raw = vm.Read32(pteAddress, Privilege::Supervisor);
	return pte;
}

uint32_t GetPTEAddress(
	const PhysicalMemory& pm,
	const VirtualMemory& vm,
	const uint32_t virtualPageNumber)
{
	const uint32_t pageDirAddress = vm.GetPageTableAddress();
	if (pageDirAddress == 0)
	{
		return UINT32_MAX;
	}

	const uint32_t pageDirIndex = virtualPageNumber >> 10;
	const uint32_t pageTableIndex = virtualPageNumber & 0x3FF;

	const uint32_t pdeAddress = pageDirAddress + pageDirIndex * sizeof(PTE);
	PTE pde;
	pde.raw = pm.Read32(pdeAddress);

	if (!pde.IsPresent())
	{
		return UINT32_MAX;
	}

	const uint32_t pageTableAddress = pde.GetFrame() * PTE::PAGE_SIZE;
	const uint32_t pteAddress = pageTableAddress + pageTableIndex * sizeof(PTE);

	return pteAddress;
}
