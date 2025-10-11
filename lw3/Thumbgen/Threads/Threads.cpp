#include "Threads.h"
#include "../Image/Image.h"

#include <stb_image_write.h>
#include <stb_image_resize2.h>
#include <iostream>
#include <filesystem>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

constexpr int RGB = 3;
constexpr std::string IMG_FILE_ADDITION = "_thumb.png";

bool ScaleImage(
	const std::string& inputPath,
	const std::string& outputDir,
	int dstWidth,
	int dstHeight);

void ScaleImages(
	const std::vector<std::filesystem::path>& images,
	const std::string& outputDir,
	const int sdtWidth,
	const int dstHeight,
	const int threadsCount = 1)
{
	int success = 0;
	int failure = 0;
	std::mutex mutex;

	boost::asio::thread_pool threadPool(threadsCount);
	for (const auto& path : images)
	{
		boost::asio::post(threadPool, [&, path] {
			const auto res = ScaleImage(path, outputDir, sdtWidth, dstHeight);

			std::lock_guard lock(mutex);
			if (res)
			{
				success++;
			}
			else
			{
				failure++;
			}
		});
	}
	threadPool.join();

	std::cout << "Success: " << success << std::endl;
	std::cout << "Failure: " << failure << std::endl;
}

std::filesystem::path GetOutputPath(const std::string& inputPath, const std::string& outputDir)
{
	const std::filesystem::path inputFilePath(inputPath);
	const std::filesystem::path outputFilePath = std::filesystem::path(outputDir) / inputFilePath.relative_path();

	const std::string stem = outputFilePath.stem().string();
	return outputFilePath.parent_path() / (stem + IMG_FILE_ADDITION);
}

bool ScaleImage(
	const std::string& inputPath,
	const std::string& outputDir,
	const int dstWidth,
	const int dstHeight)
{
	int width;
	int height;
	int channels;
	const Image image(
		inputPath.c_str(),
		width,
		height,
		channels,
		RGB);
	const auto data = image.GetData();

	if (!data)
	{
		std::cerr << "Failed to load image: " << inputPath << std::endl;
		return false;
	}
	const auto outputFilePath = GetOutputPath(inputPath, outputDir);

	std::filesystem::create_directories(outputFilePath.parent_path());

	if (width == dstWidth && height == dstHeight)
	{
		const bool success = stbi_write_png(
			outputFilePath.string().c_str(),
			dstWidth,
			dstHeight,
			RGB,
			data,
			0);
		return success;
	}

	std::vector<unsigned char> resizedData(dstWidth * dstHeight * RGB);

	const auto result = stbir_resize_uint8_linear(
		data,
		width,
		height,
		0,
		resizedData.data(),
		dstWidth,
		dstHeight,
		0,
		STBIR_RGB);

	if (result == nullptr)
	{
		std::cerr << "Failed to resize image: " << inputPath << std::endl;
		return false;
	}

	const bool success = stbi_write_png(
		outputFilePath.string().c_str(),
		dstWidth,
		dstHeight,
		RGB,
		resizedData.data(),
		dstWidth * RGB
	);

	if (!success)
	{
		std::cerr << "Failed to save thumbnail: " << outputFilePath << std::endl;
		return false;
	}

	return true;
}