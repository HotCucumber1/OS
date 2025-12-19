#pragma once
#include <cstdint>
#include <vector>

class ReferenceCollector
{
public:
	void AddReference(const uint32_t pageNumber)
	{
		m_references.push_back(pageNumber);
	}

	std::vector<uint32_t> GetReferences() const
	{
		return m_references;
	}

	void Reset()
	{
		m_references.clear();
	}

private:
	std::vector<uint32_t> m_references;
};