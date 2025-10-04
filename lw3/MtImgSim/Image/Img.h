#pragma once

#include <string>
#include <vector>

namespace Img
{
struct ImgData
{
	std::string path;
	double mse;

	bool operator<(const ImgData& other) const
	{
		return mse < other.mse;
	}
};

struct Image
{
	int width = 0;
	int height = 0;
	std::vector<float> pixels;
};

std::vector<float> SrgbToLinear(const unsigned char* srgbPixels, int pixelsCount);
double GetMse(const Image& query, const Image& candidate);
Image GetImage(const std::string& path);
} // namespace Img
