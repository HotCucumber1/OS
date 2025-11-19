#pragma once

class MockOSHandler : public IOSHandler
{
public:
	struct PageFaultCall
	{
		uint32_t virtualPageNumber;
		Access access;
		PageFaultReason reason;
		bool shouldResolve;
	};

	std::vector<PageFaultCall> pageFaultCalls;

	bool OnPageFault(
		VirtualMemory& vm,
		const uint32_t virtualPageNumber,
		const Access access,
		const PageFaultReason reason) override
	{
		pageFaultCalls.emplace_back(
			PageFaultCall{
				virtualPageNumber,
				access,
				reason,
				false });
		return false;
	}

	void ClearCalls()
	{
		pageFaultCalls.clear();
	}
};