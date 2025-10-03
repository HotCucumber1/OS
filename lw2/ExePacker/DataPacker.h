#pragma once
#include <vector>

class DataPacker
{
public:
	static std::vector<char> PackData(const std::vector<char>& srcData);
	static std::vector<char> UnpackData(const std::vector<char>& srcData, size_t originalSize);
};
