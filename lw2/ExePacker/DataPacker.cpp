#include "DataPacker.h"

#include <iostream>
#include <zlib.h>

std::vector<char> DataPacker::PackData(const std::vector<char>& srcData)
{
	if (srcData.empty())
	{
		return {};
	}

	auto destLen = compressBound(srcData.size());

	std::vector<char> destData(destLen);
	const auto res = compress2(
		reinterpret_cast<Bytef*>(destData.data()),
		&destLen,
		reinterpret_cast<const Bytef*>(srcData.data()),
		srcData.size(),
		Z_DEFAULT_COMPRESSION);

	if (res != Z_OK)
	{
		throw std::runtime_error("Z-lib pack error");
	}
	destData.resize(destLen);

	return destData;
}

std::vector<char> DataPacker::UnpackData(const std::vector<char>& compressedData, size_t originalSize)
{
	if (compressedData.empty())
	{
		return {};
	}
	std::vector<char> decompressedData(originalSize);

	uLongf destLen = originalSize;

	const auto res = uncompress(
		reinterpret_cast<Bytef*>(decompressedData.data()),
		&destLen,
		reinterpret_cast<const Bytef*>(compressedData.data()),
		compressedData.size()
	);
	if (res != Z_OK)
	{

		throw std::runtime_error("Z-lib unpack error");
	}

	if (destLen != originalSize)
	{
		decompressedData.resize(destLen);
		std::cout << "Decompressed size is not equal to " << originalSize << std::endl;
	}

	return decompressedData;
}
