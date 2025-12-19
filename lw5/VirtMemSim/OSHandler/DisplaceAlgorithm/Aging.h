#pragma once
#include "DisplaceAlgorithmInterface.h"

#include <unordered_map>
#include <vector>

class Aging final : public DisplaceAlgorithmInterface
{
public:
	explicit Aging(size_t framesNumber);

	void AccessPage(uint32_t virtualPageNumber, bool isWrite, uint32_t frame) override;

	uint32_t FindVictim() override;

	void RemovePage(uint32_t virtualPageNumber) override;

	void DoAge() override;

	bool IsDirty(uint32_t frameIndex) override;

	bool IsFrameOccupied(uint32_t frame) const override;

private:
	void AddNewPage(uint32_t virtualPageNumber, uint32_t frameIndex, bool isWrite);

	size_t m_framesNumber;
	std::vector<uint8_t> m_ageCounters;
	std::vector<bool> m_referencedPages;
	std::vector<bool> m_dirtyPages;
	std::vector<uint32_t> m_indexToPageNumber;
	std::unordered_map<uint32_t, uint32_t> m_pageNumberToIndex;
};
