#pragma once
#include <cstdint>

class VirtualMemory;

enum class PageFaultReason
{
	NotPresent,
	WriteToReadOnly,
	ExecOnNX,
	UserAccessToSupervisor,
	PhysicalAccessOutOfRange,
	MisalignedAccess,
};

enum class Access
{
	Read,
	Write,
};

class IOSHandler
{
public:
	virtual bool OnPageFault(
		VirtualMemory& vm,
		uint32_t virtualPageNumber,
		Access access,
		PageFaultReason reason) = 0;
	// Разместите здесь прочие методы-обработчики, если нужно

	virtual ~IOSHandler() = default;
};