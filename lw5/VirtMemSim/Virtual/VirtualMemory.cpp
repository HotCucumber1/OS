#include "VirtualMemory.h"

#include <stdexcept>

constexpr uint32_t PAGE_DIR_SHIFT = 22;
constexpr uint32_t PAGE_OFFSET_MASK = 0xFFF;
constexpr uint32_t PAGE_TABLE_INDEX_MASK = 0x3FF;

void UpdateAccessBits(PTE& pte, bool isWrite);

VirtualMemory::VirtualMemory(PhysicalMemory& physicalMem, IOSHandler& handler)
	: m_physicalMem(physicalMem)
	, m_handler(handler)
{
}

void VirtualMemory::SetPageTableAddress(const uint32_t physicalAddress)
{
	if (physicalAddress % PTE::PAGE_SIZE != 0)
	{
		m_handler.OnPageFault(*this, physicalAddress, Access::Read, PageFaultReason::MisalignedAccess);
	}
	m_pageDirectoryAddress = physicalAddress;
}

uint32_t VirtualMemory::GetPageTableAddress() const noexcept
{
	return m_pageDirectoryAddress;
}

uint8_t VirtualMemory::Read8(const uint32_t address, const Privilege privilege, const bool execute)
{
	const auto physAddr = TranslateAddress(address, privilege, false, execute);
	return m_physicalMem.Read8(physAddr);
}

uint16_t VirtualMemory::Read16(const uint32_t address, const Privilege privilege, const bool execute)
{
	if (address % 2 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, false, execute);
	return m_physicalMem.Read16(physAddr);
}

uint32_t VirtualMemory::Read32(const uint32_t address, const Privilege privilege, const bool execute)
{
	if (address % 4 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, false, execute);
	return m_physicalMem.Read32(physAddr);
}

uint64_t VirtualMemory::Read64(const uint32_t address, const Privilege privilege, const bool execute)
{
	if (address % 8 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, false, execute);
	return m_physicalMem.Read64(physAddr);
}

void VirtualMemory::Write8(const uint32_t address, const uint8_t value, const Privilege privilege)
{
	const auto physAddr = TranslateAddress(address, privilege, true);
	m_physicalMem.Write8(physAddr, value);
}

void VirtualMemory::Write16(const uint32_t address, const uint16_t value, const Privilege privilege)
{
	// TODO тесты доработать на выравнивание
	if (address % 2 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, true);
	m_physicalMem.Write16(physAddr, value);
}

void VirtualMemory::Write32(const uint32_t address, const uint32_t value, const Privilege privilege)
{
	if (address % 4 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, true);
	m_physicalMem.Write32(physAddr, value);
}

void VirtualMemory::Write64(const uint32_t address, const uint64_t value, const Privilege privilege)
{
	if (address % 8 != 0)
	{
		m_handler.OnPageFault(*this, address, Access::Read, PageFaultReason::MisalignedAccess);
	}
	const auto physAddr = TranslateAddress(address, privilege, true);
	m_physicalMem.Write64(physAddr, value);
}

uint32_t VirtualMemory::TranslateAddress(
	const uint32_t virtualAddress,
	const Privilege privilege,
	const bool isWrite,
	const bool isExecute)
{
	if (m_pageDirectoryAddress == 0)
	{
		return virtualAddress;
	}

	// TODO вынести смещения в константы
	const uint32_t pageDirIndex = (virtualAddress >> PAGE_DIR_SHIFT) & PAGE_TABLE_INDEX_MASK;
	const uint32_t pageTableIndex = (virtualAddress >> PTE::FRAME_SHIFT) & PAGE_TABLE_INDEX_MASK;
	const uint32_t pageOffset = virtualAddress & PAGE_OFFSET_MASK;

	const uint32_t pdeAddr = m_pageDirectoryAddress + pageDirIndex * sizeof(PTE);
	auto pde = GetPTE(pdeAddr, virtualAddress, privilege, isWrite, isExecute, true);

	if (!pde.IsAccessed())
	{
		pde.SetAccessed(true);
		WritePTE(pdeAddr, pde);
	}

	const uint32_t pageTableAddr = pde.GetFrame() * PTE::PAGE_SIZE;
	const uint32_t pteAddr = pageTableAddr + pageTableIndex * sizeof(PTE);
	auto pte = GetPTE(pteAddr, virtualAddress, privilege, isWrite, isExecute, false);

	UpdateAccessBits(pte, isWrite);
	WritePTE(pteAddr, pte);
	return pte.GetFrame() * PTE::PAGE_SIZE + pageOffset;
}

PTE VirtualMemory::GetPTE(
	const uint32_t pteAddr,
	const uint32_t virtualAddress,
	const Privilege privilege,
	const bool isWrite,
	const bool isExecute,
	const bool isPDE)
{
	auto pte = ReadPTE(pteAddr);

	if (IsPageFaultHandled(pte, virtualAddress, privilege, isWrite, isExecute, isPDE))
	{
		pte = ReadPTE(pteAddr);
		if (IsPageFaultHandled(pte, virtualAddress, privilege, isWrite, isExecute, isPDE))
		{
			throw std::runtime_error("Page Table fault not resolved after handler");
		}
	}
	return pte;
}

PTE VirtualMemory::ReadPTE(const uint32_t physicalAddress) const
{
	PTE pte;
	pte.raw = m_physicalMem.Read32(physicalAddress);
	return pte;
}

void VirtualMemory::WritePTE(const uint32_t physicalAddress, const PTE& pte) const
{
	m_physicalMem.Write32(physicalAddress, pte.raw);
}

bool VirtualMemory::IsPageFaultHandled( // TODO лучше еще раз подумать по имени
	const PTE& pte,
	const uint32_t virtualAddress,
	const Privilege privilege,
	const bool isWrite,
	const bool isExecute,
	const bool isPDE) // TODO судя по неймингу метод должен быть константным
{
	if (!pte.IsPresent())
	{
		m_handler.OnPageFault(*this, virtualAddress, isWrite ? Access::Write : Access::Read, PageFaultReason::NotPresent);
		return true;
	}

	if (isPDE)
	{
		if (privilege == Privilege::User && !pte.IsUser())
		{
			m_handler.OnPageFault(
				*this,
				virtualAddress >> PTE::FRAME_SHIFT,
				isWrite ? Access::Write : Access::Read,
				PageFaultReason::UserAccessToSupervisor);
			throw std::runtime_error("User access to supervisor");
		}
		return false;
	}

	if (isWrite)
	{
		if (!pte.IsWritable())
		{
			m_handler.OnPageFault(*this, virtualAddress, Access::Write, PageFaultReason::WriteToReadOnly);
			return true;
		}
		if (privilege == Privilege::User && !pte.IsUser())
		{
			m_handler.OnPageFault(*this, virtualAddress, Access::Write, PageFaultReason::UserAccessToSupervisor);
			return true;
		}
	}
	else
	{
		if (isExecute && pte.IsNX())
		{
			m_handler.OnPageFault(*this, virtualAddress, Access::Read, PageFaultReason::ExecOnNX);
			return true;
		}
		if (privilege == Privilege::User && !pte.IsUser())
		{
			m_handler.OnPageFault(*this, virtualAddress, Access::Read, PageFaultReason::UserAccessToSupervisor);
			return true;
		}
	}

	return false;
}

void UpdateAccessBits(PTE& pte, const bool isWrite)
{
	pte.SetAccessed(true);

	if (isWrite)
	{
		pte.SetDirty(true);
	}
}
