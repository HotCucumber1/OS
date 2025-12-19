#pragma once
#include "../OSHandler/IOSHandler.h"
#include "../Physical/PhysicalMemory.h"
#include "../PTE/PageTableEntry.h"
#include <cstdint>

enum class Privilege
{
	User,
	Supervisor
};

class VirtualMemory
{
public:
	explicit VirtualMemory(PhysicalMemory& physicalMem, IOSHandler& handler);

	void SetPageTableAddress(uint32_t physicalAddress);
	[[nodiscard]] uint32_t GetPageTableAddress() const noexcept;

	[[nodiscard]] uint8_t Read8(uint32_t address, Privilege privilege, bool execute = false);
	[[nodiscard]] uint16_t Read16(uint32_t address, Privilege privilege, bool execute = false);
	[[nodiscard]] uint32_t Read32(uint32_t address, Privilege privilege, bool execute = false);
	[[nodiscard]] uint64_t Read64(uint32_t address, Privilege privilege, bool execute = false);

	void Write8(uint32_t address, uint8_t value, Privilege privilege);
	void Write16(uint32_t address, uint16_t value, Privilege privilege);
	void Write32(uint32_t address, uint32_t value, Privilege privilege);
	void Write64(uint32_t address, uint64_t value, Privilege privilege);

private:
	uint32_t TranslateAddress(
		uint32_t virtualAddress,
		Privilege privilege,
		bool isWrite,
		bool isExecute = false);
	PTE ReadPTE(uint32_t physicalAddress) const;
	void WritePTE(uint32_t physicalAddress, const PTE& pte) const;
	bool VerifyPageAccess(
		const PTE& pte,
		uint32_t virtualAddress,
		Privilege privilege,
		bool isWrite,
		bool isExecute,
		bool isPDE = false);
	PTE GetPTE(
		uint32_t pteAddr,
		uint32_t virtualAddress,
		Privilege privilege,
		bool isWrite,
		bool isExecute,
		bool isPDE);

	PhysicalMemory& m_physicalMem;
	IOSHandler& m_handler;
	uint32_t m_pageDirectoryAddress = 0;
};