#include "PhysicalMemory.h"
#include <stdexcept>

void AssertAlignment(uint32_t address, size_t size);

PhysicalMemory::PhysicalMemory(const PhysicalMemoryConfig& cfg)
	: m_cfg(cfg)
	, m_realMemory(cfg.numFrames * cfg.frameSize)
{
}

uint32_t PhysicalMemory::GetSize() const noexcept
{
	return m_cfg.frameSize * m_cfg.numFrames;
}

uint8_t PhysicalMemory::Read8(const uint32_t address) const
{
	AssertAlignment(address, 1);
	AssertIsNotOutOfRange(address, 1);
	return m_realMemory[address];
}

uint16_t PhysicalMemory::Read16(const uint32_t address) const
{
	AssertOperation(address, 2);
	return *reinterpret_cast<const uint16_t*>(&m_realMemory[address]);
}

uint32_t PhysicalMemory::Read32(const uint32_t address) const
{
	AssertOperation(address, 4);
	return *reinterpret_cast<const uint32_t*>(&m_realMemory[address]);
}

uint64_t PhysicalMemory::Read64(const uint32_t address) const
{
	AssertOperation(address, 8);
	return *reinterpret_cast<const uint64_t*>(&m_realMemory[address]);
}

void PhysicalMemory::Write8(const uint32_t address, const uint8_t value)
{
	AssertOperation(address, 1);
	m_realMemory[address] = value;
}

void PhysicalMemory::Write16(const uint32_t address, const uint16_t value)
{
	AssertOperation(address, 2);
	*reinterpret_cast<uint16_t*>(&m_realMemory[address]) = value;
}

void PhysicalMemory::Write32(const uint32_t address, const uint32_t value)
{
	AssertOperation(address, 4);
	*reinterpret_cast<uint32_t*>(&m_realMemory[address]) = value;
}

void PhysicalMemory::Write64(const uint32_t address, const uint64_t value)
{
	AssertOperation(address, 8);
	*reinterpret_cast<uint64_t*>(&m_realMemory[address]) = value;
}

void PhysicalMemory::AssertOperation(const uint32_t address, const size_t size) const
{
	AssertAlignment(address, size);
	AssertIsNotOutOfRange(address, size);
}

void PhysicalMemory::AssertIsNotOutOfRange(const uint32_t address, const size_t size) const
{
	if (address + size > m_realMemory.size())
	{
		throw std::runtime_error("Physical access out of range");
	}
}

void AssertAlignment(const uint32_t address, const size_t size)
{
	if (address % size != 0)
	{
		throw std::runtime_error("Unaligned access");
	}
}
