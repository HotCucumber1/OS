#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "Img.h"
#include <cmath>
#include <iostream>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stdexcept>

constexpr int RGB = 3;

std::vector<float> Img::SrgbToLinear(const unsigned char* srgbPixels, const int pixelsCount)
{
	constexpr double y = 2.2;
	std::vector<float> pixels;

	for (int i = 0; i < pixelsCount; ++i)
	{
		const auto linearPixel = std::pow(srgbPixels[i] / 255.0, y);
		pixels.push_back(static_cast<float>(linearPixel));
	}
	return pixels;
}

double Img::GetMse(const Image& query, const Image& candidate)
{
	if (query.pixels.size() != candidate.pixels.size() || query.width != candidate.width || query.height != candidate.height)
	{
		throw std::runtime_error("Image size must be equal");
	}

	double totalSe = 0;
	for (int i = 0; i < query.pixels.size(); ++i)
	{
		const double diff = (query.pixels[i] - candidate.pixels[i]) * 255;
		totalSe += diff * diff;
	}

	return totalSe / query.pixels.size();
}

Img::Image Img::GetImage(const std::string& path)
{
	constexpr int targetSize = 256;

	int width;
	int height;
	unsigned char* data = stbi_load(
		path.c_str(),
		&width,
		&height,
		nullptr,
		RGB);
	if (!data)
	{
		throw std::ios_base::failure("Failed to read img: " + path);
	}

	Image img;
	img.width = width;
	img.height = height;

	const int pixelsCount = width * height * RGB;
	const auto linearPixels = SrgbToLinear(data, pixelsCount);
	stbi_image_free(data);

	img.pixels.resize(targetSize * targetSize * RGB);
	stbir_resize_float_linear(
		linearPixels.data(), width, height, 0,
		img.pixels.data(), targetSize, targetSize, 0,
		STBIR_RGB);

	img.width = targetSize;
	img.height = targetSize;

	return img;
}
