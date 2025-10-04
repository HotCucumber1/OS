#include "Threads.h"

#include <algorithm>
#include <thread>

void ImageWorker(
	const Img::Image& queryImage,
	const std::vector<std::filesystem::path>& pathChunk,
	std::vector<Img::ImgData>& results)
{
	for (const auto& path : pathChunk)
	{
		auto img = Img::GetImage(path.string());
		const auto mse = Img::GetMse(queryImage, img);

		results.push_back({
			path.string(),
			mse,
		});
	}
}

std::vector<Img::ImgData> ReduceResults(const std::vector<std::vector<Img::ImgData>>& results)
{
	std::vector<Img::ImgData> allResults;
	for (const auto& threadResult : results)
	{
		allResults.insert(
			allResults.end(),
			threadResult.begin(),
			threadResult.end()
			);
	}
	return allResults;
}

std::vector<Img::ImgData> Thread::GetImagesInfo(
	Img::Image& queryImg,
	const std::vector<std::filesystem::path>& images,
	const int threadsCount)
{
	std::vector<std::vector<Img::ImgData>> threadResults(threadsCount);
	{
		std::vector<std::jthread> threads;

		const int chunkSize = images.size() / threadsCount;
		const int remaining = images.size() % threadsCount;

		auto imagesIt = images.begin();

		for (int i = 0; i < threadsCount; ++i)
		{
			const auto currChunkSize = chunkSize + (i < remaining ? 1 : 0);
			std::vector<std::filesystem::path> fileChunk;

			std::move(imagesIt, imagesIt + currChunkSize, std::back_inserter(fileChunk));
			imagesIt += currChunkSize;

			threads.emplace_back(
				ImageWorker,
				queryImg,
				std::move(fileChunk),
				std::ref(threadResults[i]));

		}
	}

	std::vector<Img::ImgData> allResults = ReduceResults(threadResults);
	std::sort(allResults.begin(), allResults.end());
	return allResults;
}