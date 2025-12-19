#pragma once
#include <cstdint>

class DisplaceAlgorithmInterface
{
public:
	virtual void AccessPage(uint32_t virtualPageNumber, bool isWrite, uint32_t frame) = 0;

	virtual uint32_t FindVictim() = 0;

	virtual void RemovePage(uint32_t virtualPageNumber) = 0;

	virtual void DoAge() = 0;

	virtual bool IsDirty(uint32_t frameIndex) = 0;

	virtual bool IsFrameOccupied(uint32_t frame) const = 0;

	virtual ~DisplaceAlgorithmInterface() = default;
};
