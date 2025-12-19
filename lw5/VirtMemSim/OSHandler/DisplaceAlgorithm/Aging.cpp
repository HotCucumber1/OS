#include "Aging.h"

Aging::Aging(const size_t framesNumber)
	: m_framesNumber(framesNumber)
{
	m_ageCounters.resize(m_framesNumber);
	m_dirtyPages.resize(m_framesNumber);
	m_referencedPages.resize(m_framesNumber);
	m_indexToPageNumber.resize(m_framesNumber);
}

void Aging::AccessPage(const uint32_t virtualPageNumber, const bool isWrite, uint32_t frame)
{
	auto it = m_pageNumberToIndex.find(virtualPageNumber);
	if (it != m_pageNumberToIndex.end())
	{
		const uint32_t index = it->second;
		m_referencedPages[index] = true;
		if (isWrite)
		{
			m_dirtyPages[index] = true;
		}
		return;
	}

	AddNewPage(virtualPageNumber, frame, isWrite);
}

uint32_t Aging::FindVictim()
{
	uint32_t minAge = UINT32_MAX;
	uint32_t victimIndex = 0;

	for (size_t i = 0; i < m_framesNumber; ++i)
	{
		if (m_ageCounters[i] >= minAge)
		{
			continue;
		}
		minAge = m_ageCounters[i];
		victimIndex = i;
	}

	const uint32_t victimPageNumber = m_indexToPageNumber[victimIndex];

	m_indexToPageNumber[victimIndex] = UINT32_MAX;
	m_pageNumberToIndex.erase(victimPageNumber);
	m_ageCounters[victimIndex] = 0;
	m_referencedPages[victimIndex] = false;

	return victimIndex;
}

void Aging::RemovePage(const uint32_t virtualPageNumber)
{
	const auto it = m_pageNumberToIndex.find(virtualPageNumber);

	if (it == m_pageNumberToIndex.end())
	{
		return;
	}
	const uint32_t index = it->second;

	m_indexToPageNumber[index] = UINT32_MAX;
	m_ageCounters[index] = 0;
	m_referencedPages[index] = false;
	m_dirtyPages[index] = false;
	m_pageNumberToIndex.erase(it);
}

void Aging::DoAge()
{
	for (size_t i = 0; i < m_framesNumber; ++i)
	{
		m_ageCounters[i] >>= 1;

		if (!m_referencedPages[i])
		{
			continue;
		}
		m_ageCounters[i] |= (1 << (sizeof(uint8_t) - 1));
		m_referencedPages[i] = false;
	}
}

bool Aging::IsDirty(const uint32_t frameIndex)
{
	return frameIndex < m_framesNumber && m_dirtyPages[frameIndex];
}

bool Aging::IsFrameOccupied(const uint32_t frame) const
{
	return frame < m_framesNumber && m_indexToPageNumber[frame] != UINT32_MAX;
}

void Aging::AddNewPage(
	const uint32_t virtualPageNumber,
	const uint32_t frameIndex,
	const bool isWrite)
{
	if (frameIndex >= m_framesNumber)
	{
		return;
	}
	m_indexToPageNumber[frameIndex] = virtualPageNumber;
	m_pageNumberToIndex[virtualPageNumber] = frameIndex;
	m_ageCounters[frameIndex] = (1 << (sizeof(uint8_t) - 1));
	m_referencedPages[frameIndex] = true;
	m_dirtyPages[frameIndex] = isWrite;
}