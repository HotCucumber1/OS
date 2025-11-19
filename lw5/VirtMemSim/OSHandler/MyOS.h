#pragma once
#include "IOSHandler.h"

class MyOS final : public IOSHandler
{
	bool OnPageFault(
		VirtualMemory& vm,
		uint32_t virtualPageNumber,
		Access access,
		PageFaultReason reason
		) override;
};