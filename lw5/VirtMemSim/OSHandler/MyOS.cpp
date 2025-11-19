#include "MyOS.h"

bool MyOS::OnPageFault(
	VirtualMemory& vm,
	uint32_t virtualPageNumber,
	Access access,
	PageFaultReason reason)
{
}