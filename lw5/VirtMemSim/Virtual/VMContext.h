#pragma once
#include "VirtualMemory.h"

class VMContext
{
public:
	[[nodiscard]] uint8_t Read8(const uint32_t address, const bool execute = false) const
	{
		return m_vm->Read8(address, m_privilege, execute);
	}

	[[nodiscard]] uint16_t Read16(const uint32_t address, const bool execute = false) const
	{
		return m_vm->Read16(address, m_privilege, execute);
	}

	[[nodiscard]] uint32_t Read32(const uint32_t address, const bool execute = false) const
	{
		return m_vm->Read32(address, m_privilege, execute);
	}

	[[nodiscard]] uint64_t Read64(const uint32_t address, const bool execute = false) const
	{
		return m_vm->Read64(address, m_privilege, execute);
	}

	void Write8(const uint32_t address, const uint8_t value) const
	{
		m_vm->Write8(address, value, m_privilege);
	}

	void Write16(const uint32_t address, const uint16_t value) const
	{
		m_vm->Write16(address, value, m_privilege);
	}

	void Write32(const uint32_t address, const uint32_t value) const
	{
		m_vm->Write32(address, value, m_privilege);
	}

	void Write64(const uint32_t address, const uint64_t value) const
	{
		m_vm->Write64(address, value, m_privilege);
	}

protected:
	explicit VMContext(VirtualMemory& vm, Privilege privilege)
		: m_vm(&vm)
		, m_privilege(privilege)
	{
	}

	virtual VirtualMemory& GetVM() const noexcept = 0;

	VirtualMemory* m_vm;
	Privilege m_privilege;
};

class VMUserContext : public VMContext
{
public:
	explicit VMUserContext(VirtualMemory& vm)
		: VMContext{
			vm,
			Privilege::User
		}
	{
	}

	VirtualMemory& GetVM() const noexcept override
	{
		return *m_vm;
	}
};

class VMSupervisorContext : public VMContext
{
public:
	explicit VMSupervisorContext(VirtualMemory& vm)
		: VMContext{
			vm,
			Privilege::Supervisor
		}
	{
	}
	void SetPageTableAddress(const uint32_t physicalAddress) const
	{
		m_vm->SetPageTableAddress(physicalAddress);
	}

	uint32_t GetPageTableAddress() const noexcept
	{
		return m_vm->GetPageTableAddress();
	}

	VirtualMemory& GetVM() const noexcept override
	{
		return *m_vm;
	}
};
