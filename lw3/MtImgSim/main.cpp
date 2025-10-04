#include "Image/Img.h"
#include "Threads/Threads.h"

#include <algorithm>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <chrono>

constexpr double NANO_IN_SECOND = 1000000000;

struct Args
{
	std::string queryPath;
	std::string inputDirPath;
	int threadsCount;
	int topK;
	double threshold;
};

Args ParsArgs(int argc, char* argv[]);
std::vector<std::filesystem::path> GetImagesFromDir(const std::string& inputDirPath);
std::vector<Img::ImgData> GetTopResults(
	const std::vector<Img::ImgData>& allResults,
	double threshold,
	int topK
	);
void AssertThreads(int threadsCount);
void PrintResults(const std::vector<Img::ImgData>& results);


int main(int argc, char* argv[])
{
	try
	{
		const auto args = ParsArgs(argc, argv);
		AssertThreads(args.threadsCount);

		const auto startTime = std::chrono::high_resolution_clock::now();

		auto queryImg = Img::GetImage(args.queryPath);

		const auto images = GetImagesFromDir(args.inputDirPath);
		const auto allResults = Thread::GetImagesInfo(
			queryImg,
			images,
			args.threadsCount
			);
		const auto topResults = GetTopResults(allResults, args.threshold, args.topK);

		const auto endTime = std::chrono::high_resolution_clock::now();
		const auto time = (endTime - startTime).count();

		PrintResults(topResults);
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

Args ParsArgs(const int argc, char* argv[])
{
	cxxopts::Options options("MtImgSim");
	options.add_options()
		("j,threads", "Threads count", cxxopts::value<int>()->default_value("1"))
		("top", "Top similar", cxxopts::value<int>())
		("threshold", "MSE criteria", cxxopts::value<double>())
		("query", "Example img", cxxopts::value<std::string>())
		("input_dir", "Img directory", cxxopts::value<std::string>());
	options.parse_positional({"query", "input_dir"});

	const auto result = options.parse(argc, argv);

	return {
		result["query"].as<std::string>(),
		result["input_dir"].as<std::string>(),
		result["threads"].as<int>(),
		result["top"].as<int>(),
		result["threshold"].as<double>()
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

std::vector<Img::ImgData> GetTopResults(
	const std::vector<Img::ImgData>& allResults,
	const double threshold,
	const int topK
	)
{
	std::vector<Img::ImgData> topResults;
	for (const auto& res : allResults)
	{
		if (res.mse <= threshold)
		{
			topResults.push_back(res);
		}
	}
	if (topResults.size() > topK)
	{
		topResults.resize(topK);
	}
	return topResults;
}

void PrintResults(const std::vector<Img::ImgData>& results)
{
	std::cout << std::fixed << std::setprecision(4);
	for (const auto& [path, mse] : results)
	{
		std::cout << mse << "\t" << path << std::endl;
	}
}

void AssertThreads(const int threadsCount)
{
	if (threadsCount < 1)
	{
		throw std::invalid_argument("Threads count must be greater than 1");
	}
}