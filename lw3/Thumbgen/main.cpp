#include <algorithm>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <ranges>
#include "Threads/Threads.h"

constexpr double NANO_IN_SECOND = 1000000000;

struct Args
{
	int width;
	int height;
	std::string inputDirPath;
	std::string outputDirPath;
	int threadsCount;
};

Args ParsArgs(int argc, char* argv[]);
std::vector<std::filesystem::path> GetImagesFromDir(const std::string& inputDirPath);
void AssertThreads(int threadsCount);


int main(int argc, char* argv[])
{
	try
	{
		const auto args = ParsArgs(argc, argv);
		AssertThreads(args.threadsCount);

		const auto startTime = std::chrono::high_resolution_clock::now();

		const auto images = GetImagesFromDir(args.inputDirPath);
		ScaleImages(images, args.outputDirPath, args.width, args.height, args.threadsCount);

		const auto endTime = std::chrono::high_resolution_clock::now();
		const auto time = (endTime - startTime).count();

		std::cout << std::endl
				  << "Execution time: " << time / NANO_IN_SECOND << " sec"
				  << std::endl << std::endl;
	}
	catch (const std::ios_base::failure& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 2;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return 1;
	}
}

std::vector<std::string> SplitStr(const std::string& sizeParam, char delimiter = 'x')
{
	std::vector<std::string> sizes;

	auto range = sizeParam | std::ranges::views::split(delimiter);

	for (const auto& word : range) {
		sizes.emplace_back(word.begin(), word.end());
	}

	return sizes;
}

Args ParsArgs(const int argc, char* argv[])
{
	cxxopts::Options options("MtImgSim");
	options.add_options()
		("input_dir", "Img directory", cxxopts::value<std::string>())
		("output_dir", "Out directory", cxxopts::value<std::string>())
		("size", "Example img", cxxopts::value<std::string>())
		("j,threads", "Threads count", cxxopts::value<int>()->default_value("1"));
	options.parse_positional({"input_dir", "output_dir"});

	const auto result = options.parse(argc, argv);

	const auto sizes = result["size"].as<std::string>();
	const auto sizesItems = SplitStr(sizes, 'x');

	return {
		 std::stoi(sizesItems[0]),
		std::stoi(sizesItems[0]),
		result["input_dir"].as<std::string>(),
		result["output_dir"].as<std::string>(),
		result["threads"].as<int>(),
	};
}

std::vector<std::filesystem::path> GetImagesFromDir(const std::string& inputDirPath)
{
	const std::vector<std::string> allowedExt = {
		".png",
		".jpg",
		".jpeg"
	};

	std::vector<std::filesystem::path> images;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(inputDirPath))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}
		const auto extension = entry.path().extension().string();
		if (std::ranges::find(allowedExt, extension) != allowedExt.end())
		{
			images.emplace_back(entry.path());
		}
	}
	std::ranges::sort(images);

	return images;
}

void AssertThreads(const int threadsCount)
{
	if (threadsCount < 1)
	{
		throw std::invalid_argument("Threads count must be greater than 1");
	}
}
